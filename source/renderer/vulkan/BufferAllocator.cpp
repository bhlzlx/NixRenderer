#include "BufferAllocator.h"
#include "VkInc.h"
#include "ContextVk.h"
#include "BufferVk.h"
#include "DriverVk.h"
#include <cassert>

static_assert(sizeof(uint64_t) >= sizeof(VkBuffer), "handle must bigger than `VkBuffer`");

namespace Nix {
	Nix::BufferAllocatorVk * createStaticBufferAllocator(IContext * _context)
	{
		Nix::BufferAllocatorVk* allocator = new Nix::BufferAllocatorVk();
		ContextVk* context = (ContextVk*)_context;
		allocator->initialize(context,
			BufferType::IndexBufferType |
			BufferType::VertexBufferType |
			BufferType::ShaderStorageBufferType |
			BufferType::TexelBufferType,
			[allocator](ContextVk* _context, size_t _size, VmaAllocation& _allocation)->VkBuffer {
			VmaAllocationCreateInfo allocInfo = {}; {
				allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
			}
			VkBufferCreateInfo bufferInfo = {}; {
				bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
				bufferInfo.pNext = nullptr;
				bufferInfo.flags = 0;
				bufferInfo.usage =
					VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
					VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
					VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
					VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;
				bufferInfo.sharingMode = _context->getQueueFamilies().size() > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
				bufferInfo.queueFamilyIndexCount = static_cast<uint32_t>(_context->getQueueFamilies().size());
				bufferInfo.pQueueFamilyIndices = _context->getQueueFamilies().data();
				bufferInfo.size = _size;
			}
			allocator->m_usage = bufferInfo.usage;
			VkBuffer buffer = VK_NULL_HANDLE;
			VkResult rst = vmaCreateBuffer(_context->getVmaAllocator(), &bufferInfo, &allocInfo, &buffer, &_allocation, nullptr);
			assert(rst == VK_SUCCESS);
			return buffer;
		});
		return allocator;
	}

	Nix::BufferAllocatorVk * createDynamicBufferAllocator(IContext * _context)
	{
		Nix::BufferAllocatorVk* allocator = new Nix::BufferAllocatorVk();
		ContextVk* context = (ContextVk*)_context;
		allocator->initialize(context,
			BufferType::UniformBufferType,
			[allocator](ContextVk* _context, size_t _size, VmaAllocation& _allocation)->VkBuffer {
			VmaAllocationCreateInfo allocInfo = {}; {
				allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
			}
			VkBufferCreateInfo bufferInfo = {}; {
				bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
				bufferInfo.pNext = nullptr;
				bufferInfo.flags = 0;
				bufferInfo.usage =
					VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
					VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
				bufferInfo.sharingMode = _context->getQueueFamilies().size() > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
				bufferInfo.queueFamilyIndexCount = static_cast<uint32_t>(_context->getQueueFamilies().size());
				bufferInfo.pQueueFamilyIndices = _context->getQueueFamilies().data();
				bufferInfo.size = _size;
			}
			allocator->m_usage = bufferInfo.usage;
			VkBuffer buffer = VK_NULL_HANDLE;
			VkResult rst = vmaCreateBuffer(_context->getVmaAllocator(), &bufferInfo, &allocInfo, &buffer, &_allocation, nullptr);
			assert(rst == VK_SUCCESS);
			return buffer;
		});
		return allocator;
	}
	BufferAllocatorVk * createStagingBufferAllocator(IContext * _context)
	{
		Nix::BufferAllocatorVk* allocator = new Nix::BufferAllocatorVk();
		ContextVk* context = (ContextVk*)_context;
		allocator->initialize(context,
			BufferType::UniformBufferType,
			[allocator](ContextVk* _context, size_t _size, VmaAllocation& _allocation)->VkBuffer {
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
			allocator->m_usage = bufferInfo.usage;
			return buffer;
		});
		return allocator;
	}
	inline BufferAllocation BufferAllocatorVk::allocate(size_t _size) {
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
			vecAllocator.resize(vecAllocator.size() + 1);
			SubAllocator& p = vecAllocator.back();
			size_t HeapSizes[] = {
				4 * 1024, 64 * 1024, 1024 * 1024, 16 * 1024 * 1024
			};
			p.buffer = m_vkBufferCreator(m_context, HeapSizes[loc], p.allocation);
			// does not support uniform buffer at all!
			if (
				m_type == (uint32_t)BufferType::StagingBufferType ||
				m_type == (uint32_t)BufferType::UniformBufferType) {
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
				m_type == (uint32_t)BufferType::StagingBufferType ||
				m_type == (uint32_t)BufferType::UniformBufferType) {
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
	inline void BufferAllocatorVk::free(const BufferAllocation & _allocation) {
		if (_allocation.size < 1024 * 1024) {
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
						m_type == (uint32_t)BufferType::StagingBufferType ||
						m_type == (uint32_t)BufferType::UniformBufferType
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
}
