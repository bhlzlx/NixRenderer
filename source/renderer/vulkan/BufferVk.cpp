#include "BufferVk.h"
#include "ContextVk.h"
#include "QueueVk.h"
#include "DeferredDeletor.h"
#include <cassert>

namespace nix {

	VmaMemoryUsage MappingVmaMemoryUsage(VkBufferUsageFlags _usageFlags);

	bool BufferVk::InitMemoryAllocator(VkPhysicalDevice _physicalDevice, VkDevice _device)
	{
		VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.physicalDevice = _physicalDevice;
		allocatorInfo.device = _device;
		VkResult rst = vmaCreateAllocator(&allocatorInfo, &NixVMAAllocator);
		if (rst != VK_SUCCESS)
			return false;
		return true;
	}

	nix::BufferVk BufferVk::CreateBuffer(size_t _size, VkBufferUsageFlags _usage, ContextVk* _context )
	{

		VmaAllocationCreateInfo allocInfo = {}; {
			allocInfo.usage = MappingVmaMemoryUsage(_usage);
			allocInfo.flags = VMA_ALLOCATION_CREATE_STRATEGY_MIN_MEMORY_BIT;
			if (allocInfo.usage == VMA_MEMORY_USAGE_CPU_ONLY) {
				allocInfo.flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
			}
		}

		VkBufferCreateInfo bufferInfo = {}; {
			bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferInfo.pNext = nullptr;
			bufferInfo.flags = 0;
			bufferInfo.usage = _usage;
			bufferInfo.sharingMode = _context->getQueueFamilies().size() > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
			bufferInfo.queueFamilyIndexCount = static_cast<uint32_t>(_context->getQueueFamilies().size());
			bufferInfo.pQueueFamilyIndices = _context->getQueueFamilies().data();
			bufferInfo.size = _size;
		}

		BufferVk obj;
		VkBuffer buffer;
		VmaAllocation allocation;
		VkResult rst = vmaCreateBuffer(NixVMAAllocator, &bufferInfo, &allocInfo, &buffer, &allocation, nullptr);
		if (rst != VK_SUCCESS) {
			return std::move(obj);
		}
		//BufferVk* obj = new BufferVk;
		obj.m_buffer = buffer;
		obj.m_size = _size;
		obj.m_usage = _usage;
		obj.m_allocation = allocation;
		obj.m_raw = nullptr;
		vmaMapMemory(NixVMAAllocator, allocation, (void**)&obj.m_raw);
		//
		return std::move(obj);
	}

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
	IBuffer* ContextVk::createStableVBO(const void* _data, size_t _size) {
		auto buffer = BufferVk::CreateBuffer(_size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, this);
		if (_data) {
			buffer.uploadDataImmediatly(_data, _size, 0);
		}
		auto svb = new StableVertexBuffer(std::move(buffer));
		return svb;
	}

	IBuffer* ContextVk::createIndexBuffer(const void* _data, size_t _size) {
		auto buffer = BufferVk::CreateBuffer(_size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, this);
		buffer.uploadDataImmediatly(_data, _size, 0);
		auto dvb = new IndexBuffer(std::move(buffer));
		return dvb;
	}

	void StableVertexBuffer::setData(const void * _data, size_t _size, size_t _offset) {
		m_buffer.updateDataQueued(_data, _size, _offset);
	}

	IBuffer* ContextVk::createTransientVBO(size_t _size) {
		// transient buffer should use `persistent mapping` feature
		auto buffer = BufferVk::CreateBuffer(_size * MaxFlightCount, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, this);
		auto dvb = new TransientVBO(std::move(buffer), _size);
		return dvb;
	}

	void BufferVk::writeDataImmediatly(const void * _data, size_t _size, size_t _offset) {
		if (m_raw) { // persistent mapping
			memcpy(m_raw + _offset, _data, _size);
		}
		else
		{
			void* ptr = map();
			if (!ptr) {
				assert(false);
				return;
			}
			memcpy(((uint8_t*)ptr) +_offset, _data, _size);
			unmap();
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
		if (m_buffer && m_allocation) {
			vmaDestroyBuffer(NixVMAAllocator, m_buffer, m_allocation);
		}
	}

	void StableVertexBuffer::release()
	{
		BufferVk* movedTarget = new BufferVk(std::move(m_buffer));
		GetDeferredDeletor().destroyResource(movedTarget);
		delete this;
	}
	
	size_t TransientVBO::getSize() {
		return m_buffer.size();
	}

	void TransientVBO::setData(const void* _data, size_t _size, size_t _offset) {
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

	void TransientVBO::release()
	{
		m_buffer.unmap();
		BufferVk* movedTarget = new BufferVk(std::move(m_buffer));
		GetDeferredDeletor().destroyResource( movedTarget);
		delete this;
	}

	void IndexBuffer::setData(const void* _data, size_t _size, size_t _offset)
	{
		m_buffer.m_context->getGraphicsQueue()->updateBuffer(&m_buffer, _offset, _data, _size);
	}

	void IndexBuffer::release()
	{
		BufferVk* movedTarget = new BufferVk(std::move(m_buffer));
		GetDeferredDeletor().destroyResource(movedTarget);
		delete this;
	}
}