#include "UniformVk.h"
#include "ContextVk.h"
#include "BufferVk.h"
#include "DriverVk.h"
#include "RingBuffer.h"

namespace Nix {
	bool TransientBufferAllocator::createBuffer(TransientType _type, uint32_t _blockSize, VkBuffer& _buffer, VmaAllocation& _allocation)
	{
		VkBufferUsageFlags usageFlag = 0;
		switch (_type) {
		case UniformType:
			usageFlag = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		case StorageType:
			usageFlag = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		case TexelType:
			usageFlag = VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;
		}
		assert(usageFlag);
		VkDevice device = m_context->getDevice();
		VkBufferCreateInfo bufferInfo;
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
	void TransientBufferAllocator::initialize(ContextVk * _context, uint32_t _blockSize, TransientType _type)
	{
	}

	void TransientBufferAllocator::allocate(uint32_t _size, uint32_t & _offset, VkBuffer & _buffer)
	{
	}

}
