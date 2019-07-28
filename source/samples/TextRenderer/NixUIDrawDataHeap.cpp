#include "NixUIDrawDataHeap.h"
#include "NixUIDefine.h"
#include <cassert>

namespace Nix {

	DrawDataMemoryHeap::Allocation DrawDataMemoryHeap::allocateRects(uint32_t _rectNum) {
		uint32_t loc = _rectNum < 16 ? 0 : 1;
		std::vector< Heap >& heaps = m_heapCollection[loc];
		//
		size_t offset;
		uint16_t allocateId;
		for (auto& heap : heaps) {
			bool allocRst = heap.allocator.allocate(_rectNum * sizeof(UIVertex) * 4, offset, allocateId);
			if (allocRst) {
				DrawDataMemoryHeap::Allocation allocation;
				allocation.allocateId = allocateId;
				allocation.allocator = &heap.allocator;
				allocation.size = _rectNum * sizeof(UIVertex) * 4;
				allocation.ptr = heap.memory + offset;
				return allocation;
			}
		}
		heaps.resize(heaps.size() + 1);
		//
		Heap& heap = heaps.back();

		uint32_t heapSizes[] = {
			sizeof(UIVertex) * 512,
			sizeof(UIVertex) * 512 * 16
		};

		uint32_t minSizes[] = {
			sizeof(UIVertex),
			sizeof(UIVertex) * 16
		};
		heap.allocator.initialize(heapSizes[loc], minSizes[loc]);
		heap.memory = new uint8_t[heapSizes[loc]];
		bool allocRst = heap.allocator.allocate(_rectNum * sizeof(UIVertex) * 4, offset, allocateId);
		assert(allocRst);
		DrawDataMemoryHeap::Allocation allocation;
		allocation.allocateId = allocateId;
		allocation.allocator = &heap.allocator;
		allocation.size = _rectNum * sizeof(UIVertex) * 4;
		allocation.ptr = heap.memory + offset;
		return allocation;
	}

	void DrawDataMemoryHeap::free(const Allocation& _allocation) {
		_allocation.allocator->free(_allocation.allocateId);
	}
}