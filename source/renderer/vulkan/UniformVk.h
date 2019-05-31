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

	struct UniformAllocation 
	{
		VkBuffer		buffer;
		uint32_t		offset;
		uint32_t		unitSize;
		uint32_t		pool;
		uint8_t*		raw;
	};

	class UniformPool {
	private:
		std::vector< BufferVk* >			m_vecBuffer;
		std::vector< uint8_t* >				m_vecRaw;
		uint32_t							m_unitSize;
		uint32_t							m_unitCount;
		uint32_t							m_index;
		//
		std::vector< UniformAllocation >	m_freeList;
		ContextVk*							m_context;
	public:
		void initialize( ContextVk* _context, uint32_t _unitSize, uint32_t _unitCount, uint32_t _poolIndex);
		UniformAllocation allocate();
		void free( const UniformAllocation& _allocation );
		uint32_t unitSize() {
			return m_unitSize;
		}
	};

	class UniformAllocator {
	private:
		std::vector< UniformPool > m_vecPool;
	public:
		void initialize( ContextVk* _context );
		bool allocate( uint32_t _size, UniformAllocation& _allocation );
		void free( const UniformAllocation& _allocation );
	};
}