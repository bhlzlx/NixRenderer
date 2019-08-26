#include "BufferAllocator.h"
#include "VkInc.h"
#include "ContextVk.h"
#include "BufferVk.h"
#include "DriverVk.h"
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

	// base size -> x256 | alloc below | base size x16

	static_assert(sizeof(uint64_t) >= sizeof(VkBuffer), "handle must bigger than `VkBuffer`");
	/*
	union {
		VkBuffer b;
		uint64_t h;
	} cvt;
	*/
	class GeneralMultiLevelBufferAllocator
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

		GeneralMultiLevelBufferAllocator() {
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
						allocation.raw = p.raw + allocation.offset;
						return allocation;
					}
				}
				vecAllocator.resize( vecAllocator.size()+1 );
				SubAllocator& p = vecAllocator.back();
				size_t HeapSizes[] = {
					4*1024, 64*1024, 1024* 1024, 16 * 1024 * 1024
				};
				p.buffer = m_vkBufferCreator(m_context, HeapSizes[loc], p.allocation);
				// does not support uniform buffer at all!
				if ( 
					m_type == BufferType::VertexStreamDraw || 
					m_type == BufferType::StagingBuffer ||
					m_type == BufferType::IndexStreamDraw) {
					vmaMapMemory(m_context->getVmaAllocator(), p.allocation, (void**)&p.raw);
				}
				p.allocator.initialize(HeapSizes[loc], HeapSizes[loc] / 256);
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
				allocation.raw = p.raw;
				return allocation;
			}
			else
			{
				VmaAllocation vmaAllocation;
				VkBuffer b = m_vkBufferCreator(m_context, _size, vmaAllocation);
				uint8_t* raw;
				if (
					m_type == BufferType::UniformStreamDraw || 
					m_type == BufferType::VertexStreamDraw || 
					m_type == BufferType::StagingBuffer ||
					m_type == BufferType::IndexStreamDraw ) {
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
				allocation.raw = raw;
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
						if ( 
							m_type == BufferType::IndexStreamDraw || 
							m_type == BufferType::VertexStreamDraw || 
							m_type == BufferType::UniformStreamDraw 
							) {
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

	class UniformBufferAllocator : public IBufferAllocator {
	private:
		struct SubAllocator {
			BuddySystemAllocator allocator;
			VkBuffer buffer;
			VmaAllocation allocation;
			uint8_t* raw;
		};
		ContextVk*					m_context;
		std::vector<SubAllocator>	m_vecAllocatorCollection[2];
		uint32_t					m_uniformAlignment;
		// 256KB -> 1KB
		// 16KB -> 64�ֽ� | С��1K
	public:
		void initialize(ContextVk* _context) {
			DriverVk* driver = (DriverVk*)_context->getDriver();
			m_context = _context;
			m_uniformAlignment = (uint32_t)driver->getPhysicalDeviceProperties().limits.minUniformBufferOffsetAlignment;
			if (m_uniformAlignment < 64) {
				m_uniformAlignment = 64;
			}
		}
		// ͨ�� IBufferAllocator �̳�
		virtual BufferAllocation allocate(size_t _size) override
		{
			_size = (_size + m_uniformAlignment - 1) & ~((size_t)m_uniformAlignment - 1);
			_size = _size * MaxFlightCount;
			assert(_size < 2048 * MaxFlightCount);
			size_t loc = _size < 1024 ? 0 : 1;
			std::vector<SubAllocator>& allocators = m_vecAllocatorCollection[loc];
			BufferAllocation allocation;
			for (auto& allocator : allocators) {
				bool rst = allocator.allocator.allocate(_size, allocation.offset, allocation.allocationId);
				if (rst) {
					allocation.buffer = (uint64_t)allocator.buffer;
					allocation.raw = allocator.raw + allocation.offset;
					allocation.size = _size;
					return allocation;
				}
			}
			allocators.resize(allocators.size() + 1);
			SubAllocator& subAllocator = allocators.back();

			size_t heapSize, minSize;

			if (loc == 1) {
				heapSize = 256 * 1024;
				minSize = 1024;
			}
			else {
				heapSize = 16 * 1024;
				minSize = m_uniformAlignment;
			}
			subAllocator.allocator.initialize( heapSize, minSize);
			VmaAllocationCreateInfo allocInfo = {}; {
				allocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
			}
			VkBufferCreateInfo bufferInfo = {}; {
				bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
				bufferInfo.pNext = nullptr;
				bufferInfo.flags = 0;
				bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
				bufferInfo.sharingMode = m_context->getQueueFamilies().size() > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
				bufferInfo.queueFamilyIndexCount = static_cast<uint32_t>(m_context->getQueueFamilies().size());
				bufferInfo.pQueueFamilyIndices = m_context->getQueueFamilies().data();
				bufferInfo.size = heapSize;
			}
			VkResult rst = vmaCreateBuffer(m_context->getVmaAllocator(), &bufferInfo, &allocInfo, &subAllocator.buffer, &subAllocator.allocation, nullptr);
			assert(rst == VK_SUCCESS); 
			vmaMapMemory(m_context->getVmaAllocator(), subAllocator.allocation, (void**)& subAllocator.raw); vmaMapMemory(m_context->getVmaAllocator(), subAllocator.allocation, (void**)& subAllocator.raw);
			bool allocRst = subAllocator.allocator.allocate(_size, allocation.offset, allocation.allocationId);
			if (allocRst) {
				allocation.buffer = (uint64_t)subAllocator.buffer;
				allocation.raw = subAllocator.raw + allocation.offset;
				allocation.size = _size;
				return allocation;
			}
			return allocation;
		}

		virtual void free(const BufferAllocation& _allocation ) override
		{
			size_t loc = _allocation.size < 1024 ? 0 : 1;

			std::vector<SubAllocator>& allocators = m_vecAllocatorCollection[loc];
			BufferAllocation allocation;
			for (auto& allocator : allocators) {
				if (allocator.buffer == (VkBuffer)_allocation.buffer) {
					allocator.allocator.free(_allocation.allocationId);
					return;
				}
			}
			assert(false);
		}

		virtual BufferType type() override
		{
			return BufferType::UniformStreamDraw;
		}
	};

	Nix::IBufferAllocator* createVertexBufferGeneralAllocator(IContext* _context)
	{
		GeneralMultiLevelBufferAllocator * allocator = new GeneralMultiLevelBufferAllocator();
		ContextVk* context = (ContextVk*)_context;
		allocator->initialize(context, BufferType::VertexStreamDraw, 
		[](ContextVk * _context, size_t _size, VmaAllocation& _allocation)->VkBuffer {
			VmaAllocationCreateInfo allocInfo = {}; {
				allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
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
			assert(rst == VK_SUCCESS);
			return buffer;
		});
		return allocator;
	}

	Nix::IBufferAllocator* createVertexBufferGeneralAllocatorPM(IContext* _context)
	{
		GeneralMultiLevelBufferAllocator * allocator = new GeneralMultiLevelBufferAllocator();
		ContextVk* context = (ContextVk*)_context;
		allocator->initialize(context, BufferType::VertexStreamDraw,
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
			assert(rst == VK_SUCCESS);
			return buffer;
		});
		return allocator;
	}

	Nix::IBufferAllocator* createIndexBufferGeneralAllocator(IContext* _context)
	{
		GeneralMultiLevelBufferAllocator * allocator = new GeneralMultiLevelBufferAllocator();
		ContextVk* context = (ContextVk*)_context;
		allocator->initialize(context, BufferType::IndexDraw,
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
			assert(rst == VK_SUCCESS);
			return buffer;
		});
		return allocator;
	}

	Nix::IBufferAllocator* createIndexBufferGeneralAllocatorPM(IContext* _context)
	{
		GeneralMultiLevelBufferAllocator* allocator = new GeneralMultiLevelBufferAllocator();
		ContextVk* context = (ContextVk*)_context;
		allocator->initialize(context, BufferType::IndexStreamDraw,
			[](ContextVk* _context, size_t _size, VmaAllocation& _allocation)->VkBuffer {
				VmaAllocationCreateInfo allocInfo = {}; {
					allocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
				}
				VkBufferCreateInfo bufferInfo = {}; {
					bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
					bufferInfo.pNext = nullptr;
					bufferInfo.flags = 0;
					bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
					bufferInfo.sharingMode = _context->getQueueFamilies().size() > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
					bufferInfo.queueFamilyIndexCount = static_cast<uint32_t>(_context->getQueueFamilies().size());
					bufferInfo.pQueueFamilyIndices = _context->getQueueFamilies().data();
					bufferInfo.size = _size;
				}
				VkBuffer buffer = VK_NULL_HANDLE;
				VkResult rst = vmaCreateBuffer(_context->getVmaAllocator(), &bufferInfo, &allocInfo, &buffer, &_allocation, nullptr);
				assert(rst == VK_SUCCESS);
				return buffer;
			});
		return allocator;
	}

	IBufferAllocator* createStagingBufferGeneralAllocator(IContext* _context)
	{
		GeneralMultiLevelBufferAllocator* allocator = new GeneralMultiLevelBufferAllocator();
		ContextVk* context = (ContextVk*)_context;
		allocator->initialize(context, BufferType::StagingBuffer,
			[](ContextVk* _context, size_t _size, VmaAllocation& _allocation)->VkBuffer {
				VmaAllocationCreateInfo allocInfo = {}; {
					allocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
				}
				VkBufferCreateInfo bufferInfo = {}; {
					bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
					bufferInfo.pNext = nullptr;
					bufferInfo.flags = 0;
					bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
					bufferInfo.sharingMode = _context->getQueueFamilies().size() > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
					bufferInfo.queueFamilyIndexCount = static_cast<uint32_t>(_context->getQueueFamilies().size());
					bufferInfo.pQueueFamilyIndices = _context->getQueueFamilies().data();
					bufferInfo.size = _size;
				}
				VkBuffer buffer = VK_NULL_HANDLE;
				VkResult rst = vmaCreateBuffer(_context->getVmaAllocator(), &bufferInfo, &allocInfo, &buffer, &_allocation, nullptr);
				assert(rst == VK_SUCCESS);
				return buffer;
			});
		return allocator;
	}

	Nix::IBufferAllocator* createUniformBufferGeneralAllocator(IContext* _context)
	{
		UniformBufferAllocator* allocator = new UniformBufferAllocator();
		ContextVk* context = (ContextVk*)_context;
		allocator->initialize(context);
		return allocator;
	}


	class BufferAllocator : public IBufferAllocator {
	private:
		ContextVk*				m_context;
		BuddySystemAllocator	m_allocator;
		VkBuffer				m_buffer;
		VmaAllocation			m_allocation;
		BufferType				m_type;
		uint8_t*				m_raw;
	public:

		bool initialize( ContextVk* _context, size_t _heapSize, size_t _minSize, BufferType _type) {

			VmaAllocationCreateInfo allocInfo = {}; {
				allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
			}
			VkBufferCreateInfo bufferInfo = {}; {
				bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
				bufferInfo.pNext = nullptr;
				bufferInfo.flags = 0;
				bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
				bufferInfo.sharingMode = _context->getQueueFamilies().size() > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
				bufferInfo.queueFamilyIndexCount = static_cast<uint32_t>(_context->getQueueFamilies().size());
				bufferInfo.pQueueFamilyIndices = _context->getQueueFamilies().data();
				bufferInfo.size = _heapSize;
			}

			switch (_type) {
			case BufferType::IndexDraw: {
				allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
				bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
				break;
			}
			case BufferType::VertexStreamDraw: {
				allocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
				bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
				break;
			}
			case BufferType::VertexDraw : {
				allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
				bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
				break;
			}
			case BufferType::IndexStreamDraw: {
				allocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
				bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
				break;
			}
			default: return false;
			}
			VkBuffer buffer = VK_NULL_HANDLE;
			VkResult rst = vmaCreateBuffer(_context->getVmaAllocator(), &bufferInfo, &allocInfo, &buffer, &m_allocation, nullptr);
			assert(rst);
			if (!rst) {
				return false;
			}
			m_allocator.initialize(_heapSize, _minSize);
			return true;
		}
		// ͨ�� IBufferAllocator �̳�
		virtual BufferAllocation allocate(size_t _size) override
		{
			BufferAllocation allocation;
			bool rst = m_allocator.allocate(_size, allocation.offset, allocation.allocationId);
			if (rst) {
				allocation.buffer = (uint64_t)(m_buffer);
				allocation.raw = m_raw + allocation.offset;
			}
			else {
				allocation.buffer = 0;
				allocation.raw = nullptr;
			}
			return allocation;
		}
		virtual void free(const BufferAllocation& _allocation ) override
		{
			bool rst = m_allocator.free(_allocation.allocationId);
			assert(rst);
		}
		virtual BufferType type() override
		{
			return m_type;
		}
	};


	IBufferAllocator* createVertexBufferAllocator(IContext* _context, size_t _heapSize, size_t _minSize) {
		BufferAllocator* allocator = new BufferAllocator();
		allocator->initialize( (ContextVk*)_context, _heapSize, _minSize, BufferType::VertexDraw);
		return allocator;
	}
	IBufferAllocator* createVertexBufferAllocatorPM(IContext* _context, size_t _heapSize, size_t _minSize) {
		BufferAllocator* allocator = new BufferAllocator();
		allocator->initialize((ContextVk*)_context, _heapSize, _minSize, BufferType::VertexStreamDraw);
		return allocator;
	}
	IBufferAllocator* createIndexBufferAllocator(IContext* _context, size_t _heapSize, size_t _minSize) {
		BufferAllocator* allocator = new BufferAllocator();
		allocator->initialize((ContextVk*)_context, _heapSize, _minSize, BufferType::IndexDraw);
		return allocator;
	}
	IBufferAllocator* createIndexBufferAllocatorPM(IContext* _context, size_t _heapSize, size_t _minSize) {
		BufferAllocator* allocator = new BufferAllocator();
		allocator->initialize((ContextVk*)_context, _heapSize, _minSize, BufferType::IndexStreamDraw);
		return allocator;
	}
}



