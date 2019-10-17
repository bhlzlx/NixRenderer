#include "BufferVk.h"
#include "ContextVk.h"
#include "QueueVk.h"
#include "DeferredDeletor.h"
#include "BufferAllocator.h"
#include "DriverVk.h"
#include <cassert>

namespace Nix {

	VmaMemoryUsage MappingVmaMemoryUsage(VkBufferUsageFlags _usageFlags);
	VmaMemoryUsage MappingVmaMemoryUsage(VkBufferUsageFlags _usageFlags)
	{
		// ************************************
		// * VK_BUFFER_USAGE_TRANSFER_DST_BIT:
		// * VK_BUFFER_USAGE_TRANSFER_SRC_BIT:
		// * VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT:
		// * VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT:
		// * VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT:
		// * VK_BUFFER_USAGE_STORAGE_BUFFER_BIT:
		// * VK_BUFFER_USAGE_INDEX_BUFFER_BIT:
		// * VK_BUFFER_USAGE_VERTEX_BUFFER_BIT:
		// * VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT:
		// ************************************
		if (_usageFlags & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
			|| _usageFlags & VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT
			|| _usageFlags & VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT
			|| _usageFlags & VK_BUFFER_USAGE_TRANSFER_SRC_BIT
			)
		{
			return VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_ONLY; // maybe more fast on mobile devices
		}
		else if (_usageFlags & VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
			|| _usageFlags & VK_BUFFER_USAGE_INDEX_BUFFER_BIT
			|| _usageFlags & VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT
			|| _usageFlags & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
			)
		{
			return VmaMemoryUsage::VMA_MEMORY_USAGE_GPU_ONLY; // vertex buffers can be more fast with this format
		}
		return VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_ONLY;
	}

	IBuffer * ContextVk::createVertexBuffer(const void * _data, size_t _size)
	{
		auto allocation = m_staticBufferAllocator->allocate(_size);
		BufferVk* buffer = new BufferVk(this, allocation, m_staticBufferAllocator, BufferType::VertexBufferType, m_staticBufferAllocator->getUsage());
		return buffer;
	}

	IBuffer * ContextVk::createIndexBuffer(const void * _data, size_t _size)
	{
		auto allocation = m_staticBufferAllocator->allocate(_size);
		BufferVk* buffer = new BufferVk(this, allocation, m_staticBufferAllocator, BufferType::IndexBufferType, m_staticBufferAllocator->getUsage());
		return buffer;
	}

	IBuffer * ContextVk::createTexelBuffer(size_t _size)
	{
		auto allocation = m_staticBufferAllocator->allocate(_size);
		BufferVk* buffer = new BufferVk(this, allocation, m_staticBufferAllocator, BufferType::TexelBufferType, m_staticBufferAllocator->getUsage());
		return buffer;
	}

	IBuffer * ContextVk::createStorageBuffer(size_t _size)
	{
		auto allocation = m_staticBufferAllocator->allocate(_size);
		BufferVk* buffer = new BufferVk(this, allocation, m_staticBufferAllocator, BufferType::ShaderStorageBufferType, m_staticBufferAllocator->getUsage());
		return buffer;
	}

	IBuffer * ContextVk::createUniformBuffer(size_t _size)
	{
		const uint32_t alignment = m_driver->getPhysicalDeviceProperties().limits.minUniformBufferOffsetAlignment;
		_size = (_size + alignment - 1)&~(alignment - 1);
		_size *= MaxFlightCount;
		auto allocation = m_uniformBufferAllocator->allocate(_size);
		BufferVk* buffer = new BufferVk(this, allocation, m_staticBufferAllocator, BufferType::UniformBufferType, m_staticBufferAllocator->getUsage());
		return buffer;
	}

	BufferVk * ContextVk::createStagingBuffer(size_t _size)
	{
		auto allocation = m_stagingBufferAllocator->allocate(_size);
		BufferVk* buffer = new BufferVk(this, allocation, m_stagingBufferAllocator, BufferType::StagingBufferType, m_stagingBufferAllocator->getUsage());
		return buffer;
	}

	void BufferVk::writeDataImmediatly(const void * _data, size_t _size, size_t _offset) {
		if (m_allocation.raw) { // persistent mapping
			memcpy(m_allocation.raw + _offset, _data, _size);
		}
		else
		{
			if (!m_allocation.raw) {
				assert(false);
				return;
			}
			memcpy(((uint8_t*)m_allocation.raw) + _offset, _data, _size);
		}
	}
	void BufferVk::uploadDataImmediatly(const void * _data, size_t _size, size_t _offset) {
		assert(
			m_usage & VK_BUFFER_USAGE_VERTEX_BUFFER_BIT ||
			m_usage & VK_BUFFER_USAGE_INDEX_BUFFER_BIT ||
			m_usage & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT ||
			m_usage & VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT
		);
		m_context->getUploadQueue()->uploadBuffer(this, _offset, _size, _data);
	}
	void BufferVk::updateDataQueued(const void * _data, size_t _size, size_t _offset) {
		// update data on graphics queue
		m_context->getGraphicsQueue()->updateBuffer(this, _offset, _data, _size);
	}

	size_t BufferVk::getSize()
	{
		return m_allocation.size;
	}

	void BufferVk::updateData(const void * _data, size_t _size, size_t _offset)
	{
		if (m_allocation.raw) {
			writeDataImmediatly(_data, _size, _offset);
		}
		else {
			updateDataQueued(_data, _size, _offset);
		}
	}

	void BufferVk::initData(const void * _data, size_t _size, size_t _offset)
	{
		if (m_allocation.raw) {
			writeDataImmediatly(_data, _size, _offset);
		}
		else {
			uploadDataImmediatly(_data, _size, _offset);
		}
	}

	void BufferVk::release()
	{
		GetDeferredDeletor().destroyResource(this);
		delete this;
	}
}
