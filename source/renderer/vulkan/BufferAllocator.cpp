#include "BufferAllocator.h"
#include "UniformVk.h"
#include "VkInc.h"
#include "ContextVk.h"
#include "BufferVk.h"
#include <cassert>
#include <functional>

namespace Nix {

	// default buffer allocator
	// 16MB heap -> 64KB minimum
	// 1MB heap -> 4KB minimum
	// 64KB heap -> 256 Bytes minimum
	// 4KB heap -> 16 Bytes minimum

	// smaller than 256 bytes -> use the 4KB heap
	// smaller than 4KB -> use the 64KB heap
	// smaller than 64KB -> use the 1MB heap
	// smaller than 1MB -> use the 16MB heap
	// otherwise, use the vulkan allocating API by default!

	static_assert(sizeof(uint64_t) >= sizeof(VkBuffer), "handle must bigger than `VkBuffer`");
	/*
	union {
		VkBuffer b;
		uint64_t h;
	} cvt;
	*/
	class GenerateBufferAllocator
		: public IBufferAllocator
	{
	private:
		struct SubAllocator {
			BuddySystemAllocator allocator;
			VkBuffer buffer;
			VmaAllocation allocation;
			uint8_t* raw;
		};

		ContextVk*					m_context;
		BufferType					m_type;

		std::vector<SubAllocator>			m_allocatorCollection[4];

		std::function< VkBuffer(ContextVk * _context, size_t _size, VmaAllocation& _allocation )>
									m_vkBufferCreator;
		//std::function< IBuffer*(ContextVk * _context, const BufferAllocation& _allocation ) >
	//								m_nixBufferCreator;
		std::vector<VkBuffer>		m_noneGroupedBuffers;
		std::vector<VmaAllocation>	m_noneGroupedAllocation;
		std::vector<uint8_t*>		m_noneGroupedRawData;
	private:
		size_t locateCollectionIndex(size_t _capacity) {
			if (_capacity < 256 /*256 bytes*/) {
				return 0;
			}
			else if (_capacity < 4 * 1024/*4KB*/) {
				return 1;
			}
			else if (_capacity < 64 * 1024 /*64KB*/) {
				return 2;
			}
			else if (_capacity < 1024 * 1024/*1MB*/) {
				return 3;
			}
			else {
				return -1;
			}
		}
	public:

		GenerateBufferAllocator() {
		}

		void initialize( 
			ContextVk* _context, 
			BufferType _type,
			std::function< VkBuffer(ContextVk*, size_t, VmaAllocation&)> _vkBufferCreator
		) {
			m_context = _context;
			m_vkBufferCreator = _vkBufferCreator;
			m_type = _type;
		}

		virtual BufferAllocation allocate(size_t _size) {
			size_t loc = locateCollectionIndex(_size);
			if (loc != -1) {
				std::vector<SubAllocator>& vecAllocator = m_allocatorCollection[loc];
				for (auto& p : vecAllocator) {
					size_t offset;
					uint16_t allocateId;
					bool succeed = p.allocator.allocate(_size, offset, allocateId);
					if (succeed) {
						union {
							VkBuffer b;
							uint64_t h;
						} cvt;
						cvt.b = p.buffer;
						BufferAllocation allocation;
						allocation.buffer = cvt.h;
						allocation.allocationId = allocateId;
						allocation.offset = offset;
						allocation.size = _size;
						return allocation;
					}
				}
				SubAllocator p;
				size_t HeapSizes[] = {
					4*1024, 64*1024, 1024* 1024, 16 * 1024 * 1024
				};
				p.buffer = m_vkBufferCreator(m_context, HeapSizes[loc], p.allocation);
				if (m_type == UBO || m_type == CVBO) {
					vmaMapMemory(m_context->getVmaAllocator(), p.allocation, (void**)&p.raw);
				}
				p.allocator.initialize(HeapSizes[loc], HeapSizes[loc] / 256);
				vecAllocator.push_back(p);
				size_t offset;
				uint16_t allocateId;
				bool succeed = p.allocator.allocate(_size, offset, allocateId);
				assert(succeed);
				union {
					VkBuffer b;
					uint64_t h;
				} cvt;
				cvt.b = p.buffer;
				static_assert(sizeof(cvt.h) >= sizeof(cvt.b), "handle must bigger than `VkBuffer`");
				BufferAllocation allocation;
				allocation.buffer = cvt.h;
				allocation.allocationId = allocateId;
				allocation.offset = offset;
				allocation.size = _size;
				return allocation;
			}
			else
			{
				VmaAllocation vmaAllocation;
				VkBuffer b = m_vkBufferCreator(m_context, _size, vmaAllocation);
				uint8_t* raw;
				if (m_type == UBO || m_type == CVBO) {
					vmaMapMemory(m_context->getVmaAllocator(), vmaAllocation, (void**)&raw);
				}
				m_noneGroupedAllocation.push_back(vmaAllocation);
				m_noneGroupedBuffers.push_back(b);
				m_noneGroupedRawData.push_back(raw);
				//
				BufferAllocation allocation;
				union {
					VkBuffer b;
					uint64_t h;
				} cvt;
				cvt.b = b;
				allocation.buffer = cvt.h;
				allocation.allocationId = -1;
				allocation.offset = 0;
				allocation.size = _size;
				return allocation;
			}
			assert(false);
			return BufferAllocation();
		}

		virtual void free( const BufferAllocation& _allocation ) {
			if (_allocation.size < 1024 * 1024 ) {
				size_t loc = locateCollectionIndex(_allocation.size);
				std::vector<SubAllocator>& vecAllocators = m_allocatorCollection[loc];
				for (auto& p : vecAllocators) {
					union {
						VkBuffer b;
						uint64_t h;
					} cvt;
					cvt.b = p.buffer;
					if (cvt.h == _allocation.buffer) {
						auto freeRst = p.allocator.free(_allocation.allocationId);
						assert(freeRst);
						return;
					}
				}
			}
			else {
				size_t i = 0;
				for (; i < m_noneGroupedBuffers.size(); ++i) {
					union {
						VkBuffer b;
						uint64_t h;
					} cvt;
					cvt.b = m_noneGroupedBuffers[i];
					if (cvt.h == _allocation.buffer) {
						auto vmaAllocator = m_context->getVmaAllocator();
						vmaDestroyBuffer(vmaAllocator, cvt.b, m_noneGroupedAllocation[i]);
						//
						if (m_type == CVBO || m_type == UBO) {
							vmaUnmapMemory(vmaAllocator, m_noneGroupedAllocation[i]);
						}
						m_noneGroupedAllocation.erase(m_noneGroupedAllocation.begin() + i);
						m_noneGroupedBuffers.erase(m_noneGroupedBuffers.begin() + i);
						m_noneGroupedRawData.erase(m_noneGroupedRawData.begin() + i);
						//
						return;
					}
				}
				assert(false);
			}
		}

		BufferType type() {
			return m_type;
		}
	};

	Nix::IBufferAllocator* createVertexBufferGenerateAllocator(IContext* _context)
	{
		GenerateBufferAllocator * allocator = new GenerateBufferAllocator();
		ContextVk* context = (ContextVk*)_context;
		allocator->initialize(context, BufferType::SVBO, 
		[](ContextVk * _context, size_t _size, VmaAllocation& _allocation)->VkBuffer {
			VmaAllocationCreateInfo allocInfo = {}; {
				allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
			}
			VkBufferCreateInfo bufferInfo = {}; {
				bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
				bufferInfo.pNext = nullptr;
				bufferInfo.flags = 0;
				bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
				bufferInfo.sharingMode = _context->getQueueFamilies().size() > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
				bufferInfo.queueFamilyIndexCount = static_cast<uint32_t>(_context->getQueueFamilies().size());
				bufferInfo.pQueueFamilyIndices = _context->getQueueFamilies().data();
				bufferInfo.size = _size;
			}
			VkBuffer buffer = VK_NULL_HANDLE;
			VkResult rst = vmaCreateBuffer(_context->getVmaAllocator(), &bufferInfo, &allocInfo, &buffer, &_allocation, nullptr);
			assert(rst);
			return buffer;
		});
		return allocator;
	}

	Nix::IBufferAllocator* createPersistentMappingVertexBufferGenerateAllocator(IContext* _context)
	{
		GenerateBufferAllocator * allocator = new GenerateBufferAllocator();
		ContextVk* context = (ContextVk*)_context;
		allocator->initialize(context, BufferType::CVBO,
			[](ContextVk * _context, size_t _size, VmaAllocation& _allocation)->VkBuffer {
			VmaAllocationCreateInfo allocInfo = {}; {
				allocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
				allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
			}
			VkBufferCreateInfo bufferInfo = {}; {
				bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
				bufferInfo.pNext = nullptr;
				bufferInfo.flags = 0;
				bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
				bufferInfo.sharingMode = _context->getQueueFamilies().size() > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
				bufferInfo.queueFamilyIndexCount = static_cast<uint32_t>(_context->getQueueFamilies().size());
				bufferInfo.pQueueFamilyIndices = _context->getQueueFamilies().data();
				bufferInfo.size = _size;
			}
			VkBuffer buffer = VK_NULL_HANDLE;
			VkResult rst = vmaCreateBuffer(_context->getVmaAllocator(), &bufferInfo, &allocInfo, &buffer, &_allocation, nullptr);
			assert(rst);
			return buffer;
		});
		return allocator;
	}

	Nix::IBufferAllocator* createIndexBufferGenerateAllocator(IContext* _context)
	{
		GenerateBufferAllocator * allocator = new GenerateBufferAllocator();
		ContextVk* context = (ContextVk*)_context;
		allocator->initialize(context, BufferType::IBO,
			[](ContextVk * _context, size_t _size, VmaAllocation& _allocation)->VkBuffer {
			VmaAllocationCreateInfo allocInfo = {}; {
				allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
			}
			VkBufferCreateInfo bufferInfo = {}; {
				bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
				bufferInfo.pNext = nullptr;
				bufferInfo.flags = 0;
				bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
				bufferInfo.sharingMode = _context->getQueueFamilies().size() > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
				bufferInfo.queueFamilyIndexCount = static_cast<uint32_t>(_context->getQueueFamilies().size());
				bufferInfo.pQueueFamilyIndices = _context->getQueueFamilies().data();
				bufferInfo.size = _size;
			}
			VkBuffer buffer = VK_NULL_HANDLE;
			VkResult rst = vmaCreateBuffer(_context->getVmaAllocator(), &bufferInfo, &allocInfo, &buffer, &_allocation, nullptr);
			assert(rst);
			return buffer;
		});
		return allocator;
	}

	Nix::IBufferAllocator* createUniformBufferGenerateAllocator(IContext* _context)
	{
		return nullptr;
	}

}



