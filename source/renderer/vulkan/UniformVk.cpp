#include "UniformVk.h"
#include "ContextVk.h"
#include "BufferVk.h"
#include "DriverVk.h"

namespace Nix {

	void UniformPool::initialize(ContextVk* _context, uint32_t _unitSize, uint32_t _unitCount, uint32_t _poolIndex )
	{
		m_unitSize = _unitSize;
		m_unitCount = _unitCount;
		m_index = _poolIndex;
		m_context = _context;
	}

	void UniformPool::cleanup()
	{
		m_freeList.clear();
		for (auto buffer : m_vecBuffer) {
			buffer->unmap();
			delete buffer;
		}
	}

	Nix::UniformAllocation UniformPool::allocate()
	{
		if (m_freeList.empty()) {
			this->m_vecBuffer.resize( m_vecBuffer.size() + 1);
			uint32_t bufferSize = m_unitSize * m_unitCount * MaxFlightCount;
			BufferVk* newBuffer = new BufferVk();
			*newBuffer = std::move(BufferVk::CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, m_context));
			*m_vecBuffer.rbegin() = newBuffer;
			for (uint32_t i = 0; i < m_unitCount; ++i) {
				m_freeList.push_back({
					(const VkBuffer&)(*m_vecBuffer.back()),
					i * m_unitSize* MaxFlightCount,
					m_unitSize,
					m_index,
					m_vecBuffer.back()->raw() + (i * MaxFlightCount * m_unitSize)
					});
			}
		}
		UniformAllocation allocation = m_freeList.back();
		m_freeList.pop_back();
		return allocation;
	}

	void UniformPool::free(const UniformAllocation& _allocation)
	{
		m_freeList.push_back(_allocation);
	}

	inline UniformPool::~UniformPool() {
	}

	void UniformAllocator::initialize(ContextVk* _context)
	{
		DriverVk* driver = (DriverVk*)_context->getDriver();
		auto alignment = driver->getPhysicalDeviceProperties().limits.minUniformBufferOffsetAlignment;
		uint32_t unitSize = (uint32_t)alignment;
		m_vecPool.reserve(16);
		while (unitSize <= 2048) {
			m_vecPool.resize(m_vecPool.size() + 1);
			uint32_t unitCount = 64;
			if (unitSize == 128) {
				unitCount = 2048; // 256KB * MaxFlightCount
			}
			else if (unitSize == 256) {
				unitCount = 1024; // 256 KB * MaxFlightCount
			}
			else if (unitSize == 512) {
				unitCount = 512; // 256 KB * MaxFlightCount
			}
			else if (unitSize == 1024) {
				unitCount = 256; // 256 KB * MaxFlightCount
			}
			else if (unitSize == 2048) {
				unitCount = 128; // 256 KB * MaxFlightCount
			}
			else {
				assert(false);
				unitCount = 512;
			}
			m_vecPool.back().initialize(_context, unitSize, unitCount, (uint32_t)m_vecPool.size() - 1);
			//
			unitSize = unitSize << 1;
		}
	}

	void UniformAllocator::cleanup()
	{
		for (auto& chunk : m_vecPool) {
			chunk.cleanup();
		}
	}

	bool UniformAllocator::allocate(uint32_t _size, UniformAllocation& _allocation)
	{
		for (auto& pool : m_vecPool) {
			if (pool.unitSize() >= _size) {
				_allocation = pool.allocate();
				return true;
			}
		}
		assert(false);
		return false;
	}

	void UniformAllocator::free(const UniformAllocation& _allocation)
	{
		m_vecPool[_allocation.pool].free(_allocation);
	}

}
