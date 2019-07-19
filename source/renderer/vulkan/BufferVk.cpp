#include "BufferVk.h"
#include "ContextVk.h"
#include "QueueVk.h"
#include "DeferredDeletor.h"
#include "BufferAllocator.h"
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
	//
	IBuffer* ContextVk::createStaticVertexBuffer(const void* _data, size_t _size, IBufferAllocator* _allocator) {
		BufferAllocation allocation = _allocator->allocate(_size);
		BufferVk b(this, allocation, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
		VertexBuffer* buffer = new VertexBuffer(std::move(b));
		assert(buffer);
		if (_data) {
			(buffer->operator Nix::BufferVk&()).uploadDataImmediatly(_data, _size,0);
		}
		return buffer;
	}

	IBuffer* ContextVk::createIndexBuffer(const void* _data, size_t _size, IBufferAllocator* _allocator) {
		BufferAllocation allocation = _allocator->allocate(_size);
		BufferVk b(this, allocation, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
		IndexBuffer* buffer = new IndexBuffer(std::move(b));
		assert(buffer);
		if (_data) {
			(buffer->operator Nix::BufferVk & ()).uploadDataImmediatly(_data, _size, 0);
		}
		return buffer;
	}

	void VertexBuffer::setData(const void * _data, size_t _size, size_t _offset) {
		m_buffer.updateDataQueued(_data, _size, _offset);
	}

	IBuffer* ContextVk::createCahcedVertexBuffer(size_t _size, IBufferAllocator* _allocator) {
		// transient buffer should use `persistent mapping` feature
		BufferAllocation allocation = _allocator->allocate(_size);
		BufferVk b(this, allocation, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
		CachedVertexBuffer* buffer = new CachedVertexBuffer(std::move(b));
		assert(buffer);
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
			memcpy(((uint8_t*)m_allocation.raw) +_offset, _data, _size);
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

	BufferVk::~BufferVk()
	{
	}

	void VertexBuffer::release()
	{
		GetDeferredDeletor().destroyResource(this);
	}
	
	size_t CachedVertexBuffer::getSize() {
		return m_buffer.size();
	}

	void CachedVertexBuffer::setData(const void* _data, size_t _size, size_t _offset) {
		auto frame = m_buffer.m_context->getFrameCounter();
		if (m_frame != frame) {
			m_frame = frame;
			auto offset = m_offsets[2];
			m_offsets[2] = m_offsets[1];
			m_offsets[1] = m_offsets[0];
			m_offsets[0] = offset;
			// in fact, it's maybe a little slow
			memcpy(m_mem + m_offsets[0], m_mem + m_offsets[1], m_size);
		}
		// 'memcpy' is an expensive function call
		memcpy( m_mem + m_offsets[0] + _offset, _data, _size );
	}

	void CachedVertexBuffer::release()
	{
		GetDeferredDeletor().destroyResource(this);
	}

	void IndexBuffer::setData(const void* _data, size_t _size, size_t _offset)
	{
		m_buffer.m_context->getGraphicsQueue()->updateBuffer(&m_buffer, _offset, _data, _size);
	}

	void IndexBuffer::release()
	{
		GetDeferredDeletor().destroyResource(this);
	}
}