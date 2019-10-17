#include "UniformVk.h"
#include "ContextVk.h"
#include "BufferVk.h"
#include "DriverVk.h"
#include "RingBuffer.h"

namespace Nix {


	// ==========================================================================================================
	// ======================================== TransientBufferAllocator ========================================
	// ==========================================================================================================

	bool TransientBufferAllocator::createBuffer(TransientType _type, uint32_t _blockSize, VkBuffer& _buffer, VmaAllocation& _allocation)
	{
		VkBufferUsageFlags usageFlag = 0;
		switch (_type) {
		case UniformType:
			usageFlag = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
		case StorageType:
			usageFlag = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;
		}
		assert(usageFlag);
		VkDevice device = m_context->getDevice();
		//bufferInfo.flags
		VmaAllocationCreateInfo allocInfo = {}; {
			allocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
		}
		VkBufferCreateInfo bufferInfo = {}; {
			bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferInfo.pNext = nullptr;
			bufferInfo.flags = 0;
			bufferInfo.usage = usageFlag;
			bufferInfo.sharingMode = m_context->getQueueFamilies().size() > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
			bufferInfo.queueFamilyIndexCount = static_cast<uint32_t>(m_context->getQueueFamilies().size());
			bufferInfo.pQueueFamilyIndices = m_context->getQueueFamilies().data();
			bufferInfo.size = _blockSize * MaxFlightCount;
		}
		VkBuffer buffer = VK_NULL_HANDLE;
		VkResult rst = vmaCreateBuffer( m_context->getVmaAllocator(), &bufferInfo, &allocInfo, &buffer, &_allocation, nullptr);
		if (rst) {
			return true;
		}
		return false;
	}
	
	bool TransientBufferAllocator::initialize(ContextVk * _context, uint32_t _blockSize, TransientType _type) {
		m_vecTransientBuffer.reserve(8);
		TransientBuffer buff;
		VkBuffer vkBuffer;
		VmaAllocation vmaAllocation;
		bool rst = this->createBuffer(_type, _blockSize, vkBuffer, vmaAllocation);
		if (rst) {
			buff.initialize(vkBuffer, vmaAllocation, _blockSize);
			m_vecTransientBuffer.push_back(std::move(buff));
			m_type = _type;
			m_blockSize = _blockSize;
			m_context = _context;
		}
		return rst;
	}

	void TransientBufferAllocator::allocate(uint32_t _size, uint32_t & _offset, VkBuffer & _buffer, VertexType _vertexType, VkBufferView* _bufferView)
	{
		bool rst = false;
		rst = m_vecTransientBuffer[m_vecIndex].allocate(_size, _offset, _buffer);
		if (rst) {
			if (_bufferView) {
				*_bufferView = m_vecTransientBuffer[m_vecIndex].getBufferView( m_context->getDevice(), _vertexType);
			}
			return;
		}
		++m_vecIndex;
		if (m_vecIndex == m_vecTransientBuffer.size()) {
			TransientBuffer buff;
			VkBuffer vkBuffer;
			VmaAllocation vmaAllocation;
			bool rst = this->createBuffer(m_type, m_blockSize, vkBuffer, vmaAllocation);
			if (rst) {
				buff.initialize(vkBuffer, vmaAllocation, m_blockSize);
				m_vecTransientBuffer.push_back(std::move(buff));
				m_vecTransientBuffer[m_vecIndex].initialize(vkBuffer, vmaAllocation, m_blockSize);
			}
		}
		rst = m_vecTransientBuffer[m_vecIndex].allocate(_size, _offset, _buffer);
		assert(rst&&"must be true!!!");
		if (_bufferView) {
			*_bufferView = m_vecTransientBuffer[m_vecIndex].getBufferView(m_context->getDevice(), _vertexType);
		}
	}

	bool DescriptorSetBufferAllocator::initialize(ContextVk * _context)
	{
		if (!m_uniformBuffer.initialize(_context, 1024 * 1024, TransientBufferAllocator::TransientType::UniformType)) // 1MB per chunk
		{
			return false;
		}
		if (m_storageBuffer.initialize(_context, 1024 * 1024, TransientBufferAllocator::TransientType::StorageType)) // 1MB per chunk
		{
			return false;
		}
		return true;
	}

	void DescriptorSetBufferAllocator::allocateUniform(uint32_t _size, uint32_t & _offset, VkBuffer & _buffer, VertexType _vertexType, VkBufferView * _bufferView)
	{
		m_uniformBuffer.allocate(_size, _offset, _buffer, _vertexType, _bufferView );
	}

	void DescriptorSetBufferAllocator::allocateStorage(uint32_t _size, uint32_t & _offset, VkBuffer & _buffer, VertexType _vertexType, VkBufferView * _bufferView)
	{
		m_storageBuffer.allocate(_size, _offset, _buffer, _vertexType, _bufferView);
	}
}
