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

	struct UniformAllocation 
	{
		VkBuffer		buffer;
		uint32_t		offset;
		uint8_t*		raw;
		//
		uint32_t		unitSize;
		uint32_t		pool;
	};

	class UniformPool {
	private:
		std::vector< BufferVk* >			m_vecBuffer;
		std::vector< uint8_t* >				m_vecRaw;
		uint32_t							m_offset;
		//
		std::vector< UniformAllocation >	m_freeList;
	public:
		void initialize( ContextVk* _context, uint32_t _unitSize, uint32_t _unitCount );
		UniformAllocation allocate();
		void free( const UniformAllocation& _allocation );
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