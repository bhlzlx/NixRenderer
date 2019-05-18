#pragma once
#include <cstdint>
#include <map>
#include <list>
#include <cassert>
#include <vector>
#include <NixRenderer.h>
#include "vkinc.h"

namespace nix {

	class BufferVk;
	class ContextVk;
	class UniformDynamicalAllocator;
	class UBOAllocator;

	/*class IUniformStaticAllocPool {
	public:
		virtual UBOAllocation alloc() = 0;
		virtual void free(const UBOAllocation& _alloc) = 0;
		virtual size_t unitSize() = 0;
	};*/

	class NIX_API_DECL UBOAllocatePool//:public IUniformStaticAllocPool
	{
	private:
		ContextVk* m_context;
		std::vector<BufferVk*> m_buffers;
		std::list<UBOAllocation> m_freeList;
		//
		size_t m_unitSize;
		size_t m_unitCount;
		//
		size_t m_counter;
	public:
		UBOAllocatePool(size_t _uintSize, size_t _unitCount, ContextVk* _context);
		UBOAllocation alloc();
		void free(const UBOAllocation& _alloc) {
			m_freeList.push_back(_alloc);
		}
		virtual size_t unitSize() {
			return m_unitSize;
		}
	};
	//
	class UBOAllocator
	{
	private:
		// 64; 128; 256; 512, 1024; 
		std::vector<UBOAllocatePool*> m_vecAllocPools;
		std::vector< size_t > m_vecAlignSize;
		ContextVk* m_context;
	public:
		UBOAllocator() {
		}
		void initialize( ContextVk* _context );
		UBOAllocation alloc(size_t _size);
		void free(const UBOAllocation& _alloc);
	};
}