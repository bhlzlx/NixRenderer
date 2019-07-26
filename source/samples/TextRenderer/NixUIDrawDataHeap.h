#pragma once
#include <cstdint>
#include <nix/memory/BuddySystemAllocator.h>


/**********************************************************************************************************
*	The `vertex buffer` & `index buffer` in `PrebuildDrawData` handled by control object should be allocated in a heap,
*	so that, we can get a better memory management, here we use the `buddy system allocator` management method.
**********************************************************************************************************/

/**********************************************************************************************************
*	considered that one rect need 4 vertex and one vertex should handle at lease 4 float( [x,y], [u,v] )
*	we assumed that a minimum memory heap an supply at least 8196 characters, each character possesses a rect.
*   so, we can calculate the minimum heap size 8196(character count) * 4(vertex count) * 4( float count ) * 4( float size )
*   8196 * 4 * 4 * 4 = 512 KB = 0.5MB
*   but a minimum control, use only 64 bytes, for a 512KB heap, it will take 13 level binary heap
*   so we should have a smaller heap to handle the smaller allocation
*	512 * 64 = 32 KB
***********************************************************************************************************/

namespace Nix
{
	class PrebuildBufferMemoryHeap {
	public:
		struct Allocation {
			BuddySystemAllocator* allocator;
			uint32_t				size;
			uint16_t				allocateId;
			uint8_t* ptr;
		};
	private:
		struct Heap {
			BuddySystemAllocator	allocator;
			uint8_t* memory;
		};
		std::vector< Heap >			m_heapCollection[2];
	public:
		PrebuildBufferMemoryHeap() {
		}
		
		/************************************************
		*  allocate vertex buffer of `Rectangle` type
		*  from memory
		*  0---3  4---7
		*  |\  |  |\  | 
		*  | \ |  | \ | * * * * * * *
		*  1---2  5---6
		************************************************/

		Allocation allocateRects(uint32_t _rectNum);



		/************************************************
		*  allocate vertex buffer of `Rectangle` type
		*  from memory
		*   0       3
		*  / \	   / \
		* 1---2   4---5  * * * * *
		************************************************/

		Allocation allocateTriangles(uint32_t _triNum);

		// free the allocation
		void free(const Allocation& _allocation);
	};
}