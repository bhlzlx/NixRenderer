#include <NixRenderer.h>
#include "BuddySystemAllocator.h"
#include "vk_mem_alloc.h"

namespace Nix {

// 	struct BufferAllocation {
// 		uint64_t id;
// 		uint32_t offset;
// 		uint32_t size;
// 	};
// 
// 	class BufferAllocatorVk {
// 	public:
// 		virtual BufferAllocation allocate( size_t _size );
// 	};
// 
// 	struct alignas(8) BufferAllocation {
// 		BufferAllocator*	allocator;
// 		uint16_t			capacity; // times of minimum size
// 		uint16_t			id;
// 	};
// 
// 	static_assert(sizeof(BufferAllocation) <= 16, "!!!");
// 
// 	class BufferAllocator {
// 	private:
// 		VmaAllocation			m_allocation;
// 		VkBuffer				m_buffer;
// 		BuddySystemAllocator	m_allocator;
// 	public:
// 	public:
// 		bool initialize(size_t _size);
// 		bool initialize(size_t _size, size_t _miniSize);
// 		BufferAllocation allocate(size_t _size);
//  	};

}