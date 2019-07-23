#pragma once
#include <NixRenderer.h>
#include <nix/memory/BuddySystemAllocator.h>
#include "VkInc.h"
#include "vk_mem_alloc.h"

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

	IBufferAllocator* createVertexBufferGeneralAllocator(IContext* _context );
	IBufferAllocator* createVertexBufferGeneralAllocatorPM(IContext* _context);
	IBufferAllocator* createIndexBufferGeneralAllocator(IContext* _context);
	IBufferAllocator* createStagingBufferGeneralAllocator(IContext* _context);
	IBufferAllocator* createUniformBufferGeneralAllocator(IContext* _context);

	IBufferAllocator* createVertexBufferAllocator(IContext* _context, size_t _heapSize, size_t _minSize );
	IBufferAllocator* createVertexBufferAllocatorPM(IContext* _context, size_t _heapSize, size_t _minSize);
	IBufferAllocator* createIndexBufferAllocator(IContext* _context, size_t _heapSize, size_t _minSize);
}