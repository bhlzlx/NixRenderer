#pragma once
#include <NixRenderer.h>
#include <nix/memory/BuddySystemAllocator.h>
#include "VkInc.h"
#include "vk_mem_alloc.h"
#include <functional>
#include "bufferDef.h"

namespace Nix {

	// ( 16 256 ] -> use the 4KB heap
	// ( 256 4K ] -> use the 64KB heap
	// ( 4K 64K ] -> use the 1MB heap
	// ( 64K 1M ] -> use the 16MB heap
	// otherwise, use the vulkan allocating API by default!

	// base size -> x256 | alloc below | base size x16

	class ContextVk;

	class BufferAllocatorVk
	{
	private:
		struct SubAllocator {
			BuddySystemAllocator	allocator;
			VkBuffer				buffer;
			VmaAllocation			allocation;
			uint8_t*				raw;
		};

		ContextVk*					m_context;
		uint32_t					m_type;
		std::vector<SubAllocator>	m_allocatorCollection[4];
		std::function< VkBuffer(ContextVk * _context, size_t _size, VmaAllocation& _allocation)> m_vkBufferCreator;
		std::vector<VkBuffer>		m_noneGroupedBuffers;
		std::vector<VmaAllocation>	m_noneGroupedAllocation;
		std::vector<uint8_t*>		m_noneGroupedRawData;
	private:
		//
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
		uint32_t					m_usage;
		BufferAllocatorVk() {
		}

		uint32_t getUsage() {
			return m_usage;
		}

		void initialize(
			ContextVk* _context,
			uint32_t _typeFlags,
			std::function< VkBuffer(ContextVk*, size_t, VmaAllocation&)> _vkBufferCreator
		) {
			m_context = _context;
			m_vkBufferCreator = _vkBufferCreator;
			m_type = _typeFlags;
		}

		virtual BufferAllocation allocate(size_t _size);

		virtual void free(const BufferAllocation& _allocation);

		uint32_t type() {
			return m_type;
		}
	};

	// buffer type : vertex buffer / index buffer / storage buffer / texel buffer
	// not CPU side writable / readable
	BufferAllocatorVk* createStaticBufferAllocator(IContext* _context);

	// buffer type : uniform buffer ( CPU writable buffer )
	BufferAllocatorVk* createDynamicBufferAllocator(IContext* _context);

	// 暂时先用这个，以后会用更好的方式
	BufferAllocatorVk* createStagingBufferAllocator(IContext* _context);
}
