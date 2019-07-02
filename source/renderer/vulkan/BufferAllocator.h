#include <NixRenderer.h>
#include "BuddySystemAllocator.h"
#include "vk_mem_alloc.h"

namespace Nix {

	class BufferAllocator {
	private:
		VmaAllocation			m_allocation;
		VkBuffer				m_buffer;
		BuddySystemAllocator	m_allocator;
	public:
	public:
		bool initialize(size_t _size);
		bool initialize( size_t _size, size_t _miniSize );
		uint16_t allocate( size_t _size );
	};

}