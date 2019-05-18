#include "UniformVk.h"
#include "ContextVk.h"
#include "BufferVk.h"
#include "DriverVk.h"

namespace nix {

	UBOAllocatePool::UBOAllocatePool(size_t _unitSize, size_t _unitCount, ContextVk* _context)
	{
		m_context = _context;
		m_unitSize = _unitSize;
		m_unitCount = _unitCount;
		m_counter = 0;
		auto buffer = BufferVk::CreateBuffer(_unitSize * _unitCount, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, _context);
		auto* Ptr = new BufferVk(std::move(buffer));
		m_buffers.push_back(Ptr);
	}

	UBOAllocation UBOAllocatePool::alloc()
	{
		if (m_freeList.size())
		{
			auto rt = m_freeList.back();
			m_freeList.pop_back();
			return rt;
		}
		//
		UBOAllocation rt;
		if (m_counter < m_unitCount)
		{
			auto lastBuffer = m_buffers.back();
			rt = {
				lastBuffer, // buffer object
				m_counter * m_unitSize, // offset
				m_unitSize // unit size
			};
		}
		else
		{
			auto buffer = BufferVk::CreateBuffer(m_unitSize * m_counter, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, m_context);
			auto ptr = new BufferVk(std::move(buffer));
			m_buffers.push_back(ptr);
			auto lastBuffer = m_buffers.back();
			m_counter = 0;
			rt = {
				lastBuffer, // buffer object
				0, // offset
				m_unitSize
			};
		}
		++m_counter;
		return rt;
	}

	void UBOAllocator::initialize(ContextVk* _context)
	{
		m_context = _context;
		const auto& limits = m_context->getDriver()->getPhysicalDeviceProperties().limits;
		size_t alignSize = 64;
		while (alignSize <= 1024)
		{
			if (alignSize >= limits.minUniformBufferOffsetAlignment)
			{
				UBOAllocatePool* pool = new UBOAllocatePool(
					alignSize * MaxFlightCount,
					static_cast<size_t>(1024 * 1024 * 1.5f / (alignSize * MaxFlightCount)),
					_context
				);
				m_vecAllocPools.push_back(pool);
				m_vecAlignSize.push_back(alignSize);
			}
			alignSize *= 2;
		}
	}

	UBOAllocation UBOAllocator::alloc(size_t _size)
	{
		for (size_t i = 0; i < m_vecAlignSize.size(); ++i)
		{
			if (_size <= m_vecAlignSize[i])
			{
				return m_vecAllocPools[i]->alloc();
			}
		}
		assert(false);
		return{ VK_NULL_HANDLE, 0 };
	}

	void UBOAllocator::free(const UBOAllocation& _alloc)
	{
		for (auto& allocPool : m_vecAllocPools)
		{
			if (_alloc.capacity == allocPool->unitSize())
			{
				allocPool->free(_alloc);
				return;
			}
		}
		assert(false);
	}
}
