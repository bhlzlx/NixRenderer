#include "QueueVk.h"
#include "ContextVk.h"
#include "RenderPassVk.h"
#include "BufferVk.h"
#include "SwapchainVk.h"
#include "TextureVk.h"
#include "DeferredDeletor.h"

#include <vk_mem_alloc.h>
#include <assert.h>
#include "DriverVk.h"

namespace nix {

	GraphicsQueueVk* ContextVk::createGraphicsQueue( VkQueue _id, uint32_t _family, uint32_t _index ) const {
		GraphicsQueueVk* queue = new GraphicsQueueVk();
		queue->m_context = const_cast<ContextVk*>(this);
		// assign queue object
		queue->m_queue = _id;
		queue->m_queueFamily = _family;
		queue->m_queueIndex = _index;
		//
		VkCommandPoolCreateInfo poolInfo;
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.pNext = nullptr;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		poolInfo.queueFamilyIndex = queue->m_queueFamily;
		VkCommandPool commandPool;
		// assign command pool object
		if (VK_SUCCESS != vkCreateCommandPool( m_logicalDevice, &poolInfo, nullptr, &commandPool)) {
			return nullptr;
		}
		queue->m_commandPool = commandPool;
		// alloc command buffers
		VkCommandBufferAllocateInfo bufferInfo;
		bufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		bufferInfo.pNext = nullptr;
		bufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		bufferInfo.commandPool = commandPool;
		bufferInfo.commandBufferCount = MaxFlightCount;
		VkCommandBuffer commands[MaxFlightCount];
		// rendering buffers
		vkAllocateCommandBuffers(m_logicalDevice, &bufferInfo, &commands[0]);
		for (uint32_t i = 0; i < MaxFlightCount; ++i) {
			queue->m_renderBuffers[i].m_commandBuffer = commands[i];
			queue->m_renderBuffers[i].m_contextVk = const_cast<ContextVk*>(this);
		}
		// updating buffers
		vkAllocateCommandBuffers(m_logicalDevice, &bufferInfo, &commands[0]);
		for (uint32_t i = 0; i < MaxFlightCount; ++i) {
			queue->m_updatingBuffers[i].m_commandBuffer = commands[i];
		}
		// alloc rendering fences
		for (uint32_t i = 0; i < MaxFlightCount; ++i) {
			queue->m_renderFences[i] = createFence();
			queue->m_renderFencesActived[i] = VK_FALSE;
		}
		// alloc updating fences
		for (uint32_t i = 0; i < MaxFlightCount; ++i) {
			queue->m_updatingFences[i] = createFence();
			queue->m_updatingFencesActived[i] = VK_FALSE;
		}
		// default flight index
		queue->m_flightIndex = 0;
		// get semaphores from swapchain (synchronization objects)
		queue->m_imageAvailSemaphore = m_swapchain.getImageAvailSemaphore();
		queue->m_renderCompleteSemaphore = m_swapchain.getReadyToPresentSemaphore();
		// store the host device object
		queue->m_device = m_logicalDevice;
		//
		return queue;
	}

	UploadQueueVk* ContextVk::createUploadQueue(VkQueue _id, uint32_t _family, uint32_t _index ) const {
		UploadQueueVk* queue = new UploadQueueVk();
		queue->m_context = const_cast<ContextVk*>(this);
		queue->m_uploadQueue = _id;
		queue->m_queueFamily = _family;
		queue->m_queueIndex = _index;
		//_queue->m_uploadCommandPool = 
		VkCommandPoolCreateInfo poolInfo;
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.pNext = nullptr;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		poolInfo.queueFamilyIndex = m_transferQueueID >> 16;
		VkCommandPool commandPool;
		// assign command pool object
		if (VK_SUCCESS != vkCreateCommandPool( m_logicalDevice, &poolInfo, nullptr, &commandPool)) {
			return nullptr;
		}
		queue->m_uploadCommandPool = commandPool;
		//
		VkCommandBufferAllocateInfo bufferInfo;
		bufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		bufferInfo.pNext = nullptr;
		bufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		bufferInfo.commandPool = commandPool;
		bufferInfo.commandBufferCount = 1;
		VkCommandBuffer command;
		vkAllocateCommandBuffers(m_logicalDevice, &bufferInfo, &command);
		queue->m_commandBuffer.m_commandBuffer = command;
		//
		return queue;
	}

	bool GraphicsQueueVk::intialize(SwapchainVk* _swapchain)
	{
		m_imageAvailSemaphore = _swapchain->getImageAvailSemaphore();
		m_renderCompleteSemaphore = _swapchain->getReadyToPresentSemaphore();
		//
		return true;
	}

	void GraphicsQueueVk::beginFrame(uint32_t _flightIndex)
	{
		m_flightIndex = _flightIndex;
		// commit updating buffer before rendering
		if (m_updatingBuffersActived[m_flightIndex]) {
			//
			m_updatingBuffers[m_flightIndex].end();
			//
			VkPipelineStageFlags stageFlags = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			VkSubmitInfo submit;
			submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submit.pNext = nullptr;
			submit.pCommandBuffers = &(const VkCommandBuffer&)m_updatingBuffers[m_flightIndex];
			submit.commandBufferCount = 1;
			submit.pWaitDstStageMask = &stageFlags;
			submit.pWaitSemaphores = nullptr;
			submit.waitSemaphoreCount = 0;// static_cast<uint32_t>(m_semaphoresToWait.size());
			submit.pSignalSemaphores = nullptr;
			submit.signalSemaphoreCount = 0;
			vkQueueSubmit(m_queue, 1, &submit, m_updatingFences[m_flightIndex]);
			m_updatingFencesActived[m_flightIndex] = VK_TRUE;
			m_updatingBuffersActived[m_flightIndex] = VK_FALSE;
		} else {
			if ( m_updatingFencesActived[m_flightIndex] ) {
				vkWaitForFences(m_device, 1, &m_updatingFences[m_flightIndex], VK_TRUE, uint64_t(-1));
				m_updatingFencesActived[m_flightIndex] = VK_FALSE;
			}
		}


		auto device = m_context->getDevice();
		if (m_renderFencesActived[m_flightIndex])
		{
			vkWaitForFences(device, 1, &m_renderFences[_flightIndex], VK_TRUE, uint64_t(-1));
			vkResetFences(device, 1, &m_renderFences[_flightIndex]);
		}

		if (m_screenCapture.capture && m_screenCapture.invokeFrameCount == m_context->getFrameCounter() ) {
			vkWaitForFences(device, 1, &m_screenCapture.completeFence, VK_TRUE, uint64_t(-1));
			m_screenCapture.completeCapture();
			m_screenCapture.reset();
		}

		auto cmdbuff = this->commandBuffer();
		cmdbuff->begin();
		//
		m_readyForRendering = VK_TRUE;
	}

	const CommandBufferVk* GraphicsQueueVk::commandBuffer() const
	{
		return &m_renderBuffers[m_flightIndex];
	}

	void GraphicsQueueVk::endFrame()
	{
		// if has capture action
		auto command = m_renderBuffers[m_flightIndex];
		const VkCommandBuffer& cmd = command;
		assert(!command.isEncoding());
		command.end();
		VkPipelineStageFlags pipelineStageFlag = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT;
		// 		std::vector<VkPipelineStageFlags> pipelineStageFlags;
		// 		pipelineStageFlags.assign(m_semaphoresToWait.size(), VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT);
		VkSubmitInfo submit;
		submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit.pNext = nullptr;
		submit.pCommandBuffers = &cmd;
		submit.commandBufferCount = 1;
		submit.pWaitDstStageMask = &pipelineStageFlag;
		submit.pWaitSemaphores = &m_imageAvailSemaphore;
		submit.waitSemaphoreCount = 1;// static_cast<uint32_t>(m_semaphoresToWait.size());
		if (m_screenCapture.capture && !m_screenCapture.submitted ) {
			VkSemaphore sigSems[2] = {
				m_renderCompleteSemaphore,
				m_screenCapture.waitSemaphore
			};
			submit.pSignalSemaphores = sigSems;
			submit.signalSemaphoreCount = 2;
			vkQueueSubmit(m_queue, 1, &submit, m_renderFences[m_flightIndex]);
			m_screenCapture.submitCommand(m_queue);
		}
		else {
			submit.pSignalSemaphores = &m_renderCompleteSemaphore;
			submit.signalSemaphoreCount = 1;
			vkQueueSubmit(m_queue, 1, &submit, m_renderFences[m_flightIndex]);
		}

		m_renderFencesActived[m_flightIndex] = true;
		//
		m_readyForRendering = VK_FALSE;
	}

	void GraphicsQueueVk::updateBuffer(BufferVk* _buffer, size_t _offset, const void * _data, size_t _length) {
		if (m_readyForRendering) 
		{
			auto& cmdbuff = m_updatingBuffers[m_flightIndex];
			if (!m_updatingBuffersActived[m_flightIndex]) {
				cmdbuff.begin();
				m_updatingBuffersActived[m_flightIndex] = VK_TRUE;
			}
			if (m_updatingFencesActived[m_flightIndex]) {
				vkWaitForFences(m_device, 1, &m_updatingFences[m_flightIndex], VK_TRUE, uint64_t(-1));
				m_updatingFencesActived[m_flightIndex] = VK_FALSE;
			}
			cmdbuff.updateBuffer(_buffer, _offset, _length, _data);
		}
		else
		{
			m_renderBuffers[m_flightIndex].updateBuffer(_buffer, _offset, _length, _data);
		}
	}

// 	void GraphicsQueueVk::updateTexture(TextureVk* _texture, const ImageRegion& _region, const void * _data, size_t _length) {
// 		if (!m_readyForRendering)
// 		{
// 			auto& cmdbuff = m_updatingBuffers[m_flightIndex];
// 			if (!m_updatingBuffersActived[m_flightIndex]) {
// 				cmdbuff.begin();
// 				m_updatingBuffersActived[m_flightIndex] = VK_TRUE;
// 			}
// 			if (m_updatingFencesActived[m_flightIndex]) {
// 				vkWaitForFences(m_device, 1, &m_updatingFences[m_flightIndex], VK_TRUE, uint64_t(-1));
// 				m_updatingFencesActived[m_flightIndex] = VK_FALSE;
// 			}
// 			cmdbuff.updateTexture(_texture, _data, _length, _region);
// 		}
// 		else
// 		{
// 			m_renderBuffers[m_flightIndex].updateTexture(_texture, _data, _length, _region);
// 		}
// 	}

	void GraphicsQueueVk::updateTexture(TextureVk* _texture, const TextureRegion& _region, const void * _data, size_t _length)
	{
		if (!m_readyForRendering)
		{
			auto& cmdbuff = m_updatingBuffers[m_flightIndex];
			if (!m_updatingBuffersActived[m_flightIndex]) {
				cmdbuff.begin();
				m_updatingBuffersActived[m_flightIndex] = VK_TRUE;
			}
			if (m_updatingFencesActived[m_flightIndex]) {
				vkWaitForFences(m_device, 1, &m_updatingFences[m_flightIndex], VK_TRUE, uint64_t(-1));
				m_updatingFencesActived[m_flightIndex] = VK_FALSE;
			}
			cmdbuff.updateTexture(_texture, _data, _length, _region);
		}
		else
		{
			m_renderBuffers[m_flightIndex].updateTexture(_texture, _data, _length, _region);
		}
	}

	void GraphicsQueueVk::updateTexture(TextureVk* _texture, const TextureRegion& _region, uint32_t _mipCount, const void * _data, size_t _length) 
	{
		if (!m_readyForRendering)
		{
			auto& cmdbuff = m_updatingBuffers[m_flightIndex];
			if (!m_updatingBuffersActived[m_flightIndex]) {
				cmdbuff.begin();
				m_updatingBuffersActived[m_flightIndex] = VK_TRUE;
			}
			if (m_updatingFencesActived[m_flightIndex]) {
				vkWaitForFences(m_device, 1, &m_updatingFences[m_flightIndex], VK_TRUE, uint64_t(-1));
				m_updatingFencesActived[m_flightIndex] = VK_FALSE;
			}
			cmdbuff.updateTexture(_texture, _data, _length, _region, _mipCount);
		}
		else
		{
			m_renderBuffers[m_flightIndex].updateTexture(_texture, _data, _length, _region, _mipCount);
		}
	}

	void GraphicsQueueVk::captureScreen(TextureVk* _texture, void * _raw, size_t _length, FrameCaptureCallback _callback, IFrameCapture* _capture )
	{
		m_screenCapture.initialize( m_context, m_commandPool, _texture, _raw, _length, _callback, _capture);
	}

	void GraphicsQueueVk::waitForIdle() const
	{
		vkQueueWaitIdle(m_queue);
	}

	void CommandBufferVk::updateBuffer(BufferVk* _buffer, size_t _offset, size_t _size, const void* _data)
	{
		BufferVk stageBuffer = BufferVk::CreateBuffer(_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, m_contextVk);
		stageBuffer.writeDataImmediatly(_data,_size,0);
		VkBufferCopy copy;
		copy.srcOffset = 0;
		copy.dstOffset = _offset;
		copy.size = _size;
		VkBufferMemoryBarrier barrierBefore; {
			barrierBefore.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
			barrierBefore.srcAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
			barrierBefore.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrierBefore.pNext = nullptr;
			barrierBefore.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrierBefore.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrierBefore.offset = _offset;
			barrierBefore.size = _size;
			barrierBefore.buffer = *_buffer;
		}
		VkBufferMemoryBarrier barrierAfter; {
			barrierAfter.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
			barrierAfter.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrierAfter.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
			barrierAfter.pNext = nullptr;
			barrierAfter.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrierAfter.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrierAfter.offset = _offset;
			barrierAfter.size = _size;
			barrierAfter.buffer = *_buffer;
		}
		vkCmdPipelineBarrier(m_commandBuffer, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
			0, nullptr,
			1, &barrierBefore,
			0, nullptr);
		vkCmdCopyBuffer(m_commandBuffer, (const VkBuffer&)stageBuffer, *_buffer, 1, &copy);
		vkCmdPipelineBarrier(m_commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0,
			0, nullptr,
			1, &barrierAfter,
			0, nullptr);
		//
		auto deletor = GetDeferredDeletor();
		auto* Tbuffer = new BufferVk(std::move(stageBuffer));
		deletor.destroyResource(Tbuffer);
	}
	/*
	void CommandBufferVk::updateTexture(TextureVk* _texture, const void* _data, size_t _length, const ImageRegion& _region) const
	{
	auto context = (ContextVk*)GetContextVulkan();
	_length = _length < 64 ? 64 : _length;
	auto stageBuffer = context->createBuffer(_length, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
	stageBuffer->writeDataCPUAccess(_data, 0, _length);

	std::vector<VkBufferImageCopy> copies;

	uint32_t width = _region.size.width, height = _region.size.height;
	int32_t offsetx = _region.offset.x, offsety = _region.offset.y;

	uint32_t baseMipLevel = _region.baseMipLevel;
	uint32_t baseLayer = _region.offset.z;
	uint32_t layerCount = _region.size.depth;

	for (uint32_t i = 0; i< _region.mipLevelCount; ++i) // one `copy` object can only handle one `mipmap level`
	{
	VkBufferImageCopy copy; {
	copy.imageOffset = { // for 2d texture, base z offset must be 0
	offsetx, offsetx, 0
	};
	copy.imageExtent = {
	width, height, 1 // depth count : for 2d texture must be 1
	};
	copy.imageSubresource = {
	VK_IMAGE_ASPECT_COLOR_BIT, // only color texture supported
	baseMipLevel,
	baseLayer,
	layerCount //
	};
	copy.bufferOffset = 0;
	copy.bufferImageHeight = 0;
	copy.bufferRowLength = 0;
	}
	copies.push_back(copy);
	width >>= 1; height >>= 1;
	offsetx >>= 1; offsety >>= 1;
	++baseMipLevel;
	}
	//

	//
	const VkImageMemoryBarrier barrierBefore = {
	VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, // sType
	nullptr, // pNext
	VK_ACCESS_SHADER_READ_BIT, // srcAccessMask
	VK_ACCESS_TRANSFER_WRITE_BIT, // dstAccessMask
	_texture->getImageLayout(), // oldLayout
	VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, // newLayout
	VK_QUEUE_FAMILY_IGNORED, // srcQueueFamilyIndex
	VK_QUEUE_FAMILY_IGNORED, // dstQueueFamilyIndex
	_texture->getImage(), // image
	{ // subresourceRange
	VK_IMAGE_ASPECT_COLOR_BIT, // aspectMask
	_region.baseMipLevel, // baseMipLevel
	_region.mipLevelCount, // levelCount
	_region.offset.z, // baseArrayLayer
	_region.size.depth // layerCount
	}
	};
	const VkImageMemoryBarrier barrierAfter = {
	VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, // sType
	nullptr, // pNext
	VK_ACCESS_TRANSFER_WRITE_BIT, // srcAccessMask
	VK_ACCESS_SHADER_READ_BIT, // dstAccessMask
	VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, // oldLayout
	VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, // newLayout
	VK_QUEUE_FAMILY_IGNORED, // srcQueueFamilyIndex
	VK_QUEUE_FAMILY_IGNORED, // dstQueueFamilyIndex
	_texture->getImage(), // image
	{ // subresourceRange
	VK_IMAGE_ASPECT_COLOR_BIT, // aspectMask
	_region.baseMipLevel, // baseMipLevel
	_region.mipLevelCount, // levelCount
	_region.offset.z, // baseArrayLayer
	_region.size.depth // layerCount
	}
	};
	vkCmdPipelineBarrier(
	m_commandBuffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
	0, nullptr, 0, nullptr,
	1, &barrierBefore);
	vkCmdCopyBufferToImage(m_commandBuffer, *stageBuffer, _texture->getImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<uint32_t>(copies.size()), copies.data());
	//
	vkCmdPipelineBarrier(
	m_commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
	0, nullptr, 0, nullptr,
	1, &barrierAfter);
	//
	_texture->setImageLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	//
	auto deletor = GetDeferredDeletor();
	deletor.destroy(stageBuffer);
	}*/

	void CommandBufferVk::updateTexture(TextureVk* _texture, const void* _data, size_t _length, const TextureRegion& _region) const
	{
		_length = _length < 64 ? 64 : _length;
		auto stageBuffer = BufferVk::CreateBuffer(_length, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, m_contextVk);
		stageBuffer.writeDataImmediatly(_data,_length,0);
		//
		uint32_t width = _region.size.width, height = _region.size.height;
		int32_t offsetx = _region.offset.x, offsety = _region.offset.y;

		VkBufferImageCopy copy; {
			copy.imageOffset = {
				(int32_t)_region.offset.x, (int32_t)_region.offset.y, (int32_t)_region.offset.z
			};
			copy.imageExtent = {
				_region.size.width, _region.size.height, _region.size.depth
			};
			copy.imageSubresource = {
				VK_IMAGE_ASPECT_COLOR_BIT, // only color texture supported
				_region.mipLevel,
				_region.baseLayer,
				_region.size.depth
			};
			copy.bufferOffset = 0;
			copy.bufferImageHeight = 0;
			copy.bufferRowLength = 0;
		}
		const VkImageMemoryBarrier barrierBefore = {
			VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, // sType 
			nullptr, // pNext
			VK_ACCESS_SHADER_READ_BIT, // srcAccessMask
			VK_ACCESS_TRANSFER_WRITE_BIT, // dstAccessMask
			_texture->getImageLayout(), // oldLayout
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, // newLayout
			VK_QUEUE_FAMILY_IGNORED, // srcQueueFamilyIndex 
			VK_QUEUE_FAMILY_IGNORED, // dstQueueFamilyIndex 
			_texture->getImage(), // image 
			{ // subresourceRange 
				VK_IMAGE_ASPECT_COLOR_BIT, // aspectMask 
				_region.mipLevel, // baseMipLevel 
				1, // levelCount
				_region.baseLayer + _region.offset.z, // baseArrayLayer
				_region.size.depth // layerCount
			}
		};
		const VkImageMemoryBarrier barrierAfter = {
			VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, // sType 
			nullptr, // pNext
			VK_ACCESS_TRANSFER_WRITE_BIT, // srcAccessMask
			VK_ACCESS_SHADER_READ_BIT, // dstAccessMask
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, // oldLayout
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, // newLayout
			VK_QUEUE_FAMILY_IGNORED, // srcQueueFamilyIndex 
			VK_QUEUE_FAMILY_IGNORED, // dstQueueFamilyIndex 
			_texture->getImage(), // image 
			{ // subresourceRange 
				VK_IMAGE_ASPECT_COLOR_BIT, // aspectMask 
				_region.mipLevel, // baseMipLevel 
				1, // levelCount
				_region.baseLayer + _region.offset.z, // baseArrayLayer
				_region.size.depth // layerCount
			}
		};
		vkCmdPipelineBarrier(
			m_commandBuffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
			0, nullptr, 0, nullptr,
			1, &barrierBefore);
		vkCmdCopyBufferToImage(m_commandBuffer, (const VkBuffer&)stageBuffer, _texture->getImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);
		//
		vkCmdPipelineBarrier(
			m_commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
			0, nullptr, 0, nullptr,
			1, &barrierAfter);
		//
		_texture->setImageLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		//
		auto deletor = GetDeferredDeletor();
		auto TBuffer = new BufferVk(std::move(stageBuffer));
		deletor.destroyResource(TBuffer);
	}

	void CommandBufferVk::updateTexture(TextureVk* _texture, const void* _data, size_t _length, const TextureRegion& _baseMipRegion, uint32_t _mipCount) const {
		_length = _length < 64 ? 64 : _length;
		auto stageBuffer = BufferVk::CreateBuffer(_length, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, m_contextVk);
		stageBuffer.writeDataImmediatly(_data,_length, 0);

		assert(_baseMipRegion.mipLevel == 0);
		assert(_baseMipRegion.baseLayer == 0);

		uint32_t baseWidth = _baseMipRegion.size.width, baseHeight = _baseMipRegion.size.height;
		uint32_t baseOffsetX = _baseMipRegion.offset.x, baseOffsetY = _baseMipRegion.offset.y;

		std::vector< VkBufferImageCopy > copies;
		for (uint32_t miplevel = 0; miplevel < _mipCount; ++miplevel) {
			VkBufferImageCopy copy; {
				copy.imageOffset = {
					(int32_t)baseOffsetX >> miplevel, (int32_t)baseOffsetY >> miplevel, (int32_t)_baseMipRegion.offset.z
				};
				copy.imageExtent = {
					baseWidth >> miplevel, baseHeight >> miplevel, _baseMipRegion.size.depth
				};
				copy.imageSubresource = {
					VK_IMAGE_ASPECT_COLOR_BIT, // only color texture supported
					miplevel,
					_baseMipRegion.baseLayer,
					_baseMipRegion.size.depth
				};
				copy.bufferOffset = 0;
				copy.bufferImageHeight = 0;
				copy.bufferRowLength = 0;
			}
			const VkImageMemoryBarrier barrierBefore = {
				VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, // sType 
				nullptr, // pNext
				VK_ACCESS_SHADER_READ_BIT, // srcAccessMask
				VK_ACCESS_TRANSFER_WRITE_BIT, // dstAccessMask
				_texture->getImageLayout(), // oldLayout
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, // newLayout
				VK_QUEUE_FAMILY_IGNORED, // srcQueueFamilyIndex 
				VK_QUEUE_FAMILY_IGNORED, // dstQueueFamilyIndex 
				_texture->getImage(), // image 
				{ // subresourceRange 
					VK_IMAGE_ASPECT_COLOR_BIT, // aspectMask 
					miplevel, // baseMipLevel 
					1, // levelCount
					_baseMipRegion.baseLayer + _baseMipRegion.offset.z, // baseArrayLayer
					_baseMipRegion.size.depth // layerCount
				}
			};
			const VkImageMemoryBarrier barrierAfter = {
				VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, // sType 
				nullptr, // pNext
				VK_ACCESS_TRANSFER_WRITE_BIT, // srcAccessMask
				VK_ACCESS_SHADER_READ_BIT, // dstAccessMask
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, // oldLayout
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, // newLayout
				VK_QUEUE_FAMILY_IGNORED, // srcQueueFamilyIndex 
				VK_QUEUE_FAMILY_IGNORED, // dstQueueFamilyIndex 
				_texture->getImage(), // image 
				{ // subresourceRange 
					VK_IMAGE_ASPECT_COLOR_BIT, // aspectMask 
					miplevel, // baseMipLevel 
					1, // levelCount
					_baseMipRegion.baseLayer + _baseMipRegion.offset.z, // baseArrayLayer
					_baseMipRegion.size.depth // layerCount
				}
			};

			vkCmdPipelineBarrier(
				m_commandBuffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
				0, nullptr, 0, nullptr,
				1, &barrierBefore);
			vkCmdCopyBufferToImage(m_commandBuffer, (const VkBuffer&)stageBuffer, _texture->getImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<uint32_t>(copies.size()), copies.data());
			//
			vkCmdPipelineBarrier(
				m_commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
				0, nullptr, 0, nullptr,
				1, &barrierAfter);
			//
		}
		//
		_texture->setImageLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		//
		auto deletor = GetDeferredDeletor();
		auto TBuffer = new BufferVk(std::move(stageBuffer));
		deletor.destroyResource(TBuffer);
	}

	void CommandBufferVk::getFramePixels(TextureVk* _texture, BufferVk& _stagingBuffer)
	{
		uint32_t width = _texture->getDesc().width, height = _texture->getDesc().height;
		int32_t offsetx = 0, offsety = 0;

		VkBufferImageCopy copy; {
			copy.imageOffset = {
				0, 0, 0
			};
			copy.imageExtent = {
				width, height, 1
			};
			copy.imageSubresource = {
				VK_IMAGE_ASPECT_COLOR_BIT, // only color texture supported
				0,
				0,
				1
			};
			copy.bufferOffset = 0;
			copy.bufferImageHeight = 0;
			copy.bufferRowLength = 0;
		}
		vkCmdCopyImageToBuffer(m_commandBuffer, _texture->getImage(), VK_IMAGE_LAYOUT_GENERAL, (const VkBuffer&)_stagingBuffer, 1, &copy);
	}

	void CommandBufferVk::setViewport(VkViewport _vp)
	{
		vkCmdSetViewport(m_commandBuffer, 0, 1, &_vp);
	}
	void CommandBufferVk::setScissor(VkRect2D _scissor)
	{
		vkCmdSetScissor(m_commandBuffer, 0, 1, &_scissor);
	}

	bool CommandBufferVk::begin() const
	{
		VkCommandBufferBeginInfo info;
		info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		info.pNext = nullptr;
		info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		info.pInheritanceInfo = nullptr;
		VkResult rst = vkBeginCommandBuffer(m_commandBuffer, &info);
		return rst == VK_SUCCESS;
	}

	/*void CommandBufferVk::endEncoding() 
	{
	vkEndCommandBuffer(m_commandBuffer);
	}*/

	void CommandBufferVk::end() const
	{
		vkEndCommandBuffer(m_commandBuffer);
	}

	void UploadQueueVk::uploadBuffer( BufferVk* _buffer, size_t _offset, size_t _size, const void * _data)
	{
		m_uploadMutex.lock();
		m_commandBuffer.begin();
		m_commandBuffer.updateBuffer(_buffer, _offset, _size, _data);
		m_commandBuffer.end();
		VkSubmitInfo submit;
		submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit.pNext = nullptr;
		submit.pCommandBuffers = &(const VkCommandBuffer&)m_commandBuffer;
		submit.commandBufferCount = 1;
		submit.pWaitDstStageMask = 0;
		submit.pWaitSemaphores = nullptr;
		submit.waitSemaphoreCount = 0;
		submit.signalSemaphoreCount = 0;
		//
		vkQueueSubmit(m_uploadQueue, 1, &submit, 0);
		vkQueueWaitIdle(m_uploadQueue);
		m_uploadMutex.unlock();
	}

	void UploadQueueVk::uploadTexture(TextureVk* _texture, const void* _data, size_t _length, const TextureRegion& _region)
	{
		m_uploadMutex.lock();
		m_commandBuffer.begin();
		m_commandBuffer.updateTexture(_texture, _data, _length, _region);
		m_commandBuffer.end();
		VkSubmitInfo submit;
		submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit.pNext = nullptr;
		submit.pCommandBuffers = &(const VkCommandBuffer&)m_commandBuffer;
		submit.commandBufferCount = 1;
		submit.pWaitDstStageMask = 0;
		submit.pWaitSemaphores = nullptr;
		submit.waitSemaphoreCount = 0;
		submit.signalSemaphoreCount = 0;
		//
		vkQueueSubmit(m_uploadQueue, 1, &submit, 0);
		vkQueueWaitIdle(m_uploadQueue);
		m_uploadMutex.unlock();
	}

	ScreenCapture::ScreenCapture() :
		texture(nullptr)
		, raw(nullptr)
		, length(0)
		, stagingBuffer(nullptr)
		, completeFence(VK_NULL_HANDLE)
		, waitSemaphore(VK_NULL_HANDLE)
		, commandBuffer(CommandBufferVk())
		, capture(nullptr)
		, callback(nullptr)
		, submitted( false )
	{

	}

	void ScreenCapture::initialize(ContextVk* _context, VkCommandPool _pool, TextureVk* _texture, void* _raw, size_t _length, FrameCaptureCallback _callback, IFrameCapture* _capture )
	{
		if (!_texture) {
			return;
		}
		context = _context;
		texture = _texture;
		raw = _raw;
		length = _length;
		callback = _callback;
		capture = _capture;
		if (VK_NULL_HANDLE == completeFence) {

			completeFence = context->createFence();
			waitSemaphore = context->createSemaphore();
			VkCommandBufferAllocateInfo bufferInfo;
			bufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			bufferInfo.pNext = nullptr;
			bufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			bufferInfo.commandPool = _pool;
			bufferInfo.commandBufferCount = 1;
			VkResult rst = vkAllocateCommandBuffers(context->getDevice(), &bufferInfo, &this->commandBuffer.m_commandBuffer);
			assert(rst == VK_SUCCESS);
		}
		// check the _length is match
		auto bytesTotal = KsFormatBits(texture->getDesc().format) * texture->getDesc().width * texture->getDesc().height / 8;
		assert(bytesTotal <= _length);
		stagingBuffer = new BufferVk();
		*stagingBuffer = std::move(BufferVk::CreateBuffer( bytesTotal, VkBufferUsageFlagBits::VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT | VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_DST_BIT, context));
		realLength = bytesTotal;
		this->commandBuffer.getFramePixels(texture, *stagingBuffer);
		invokeFrameCount = context->getFrameCounter() + MaxFlightCount;
	}

	void ScreenCapture::submitCommand(VkQueue _queue)
	{
		if (submitted) {
			return;
		}
		commandBuffer.begin();
		commandBuffer.getFramePixels(texture, *stagingBuffer);
		commandBuffer.end();
		VkPipelineStageFlags stageFlags = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		VkSubmitInfo submit;
		submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit.pNext = nullptr;
		submit.pCommandBuffers = &(const VkCommandBuffer&)commandBuffer;
		submit.commandBufferCount = 1;
		submit.pWaitDstStageMask = &stageFlags;
		submit.pWaitSemaphores = &waitSemaphore;
		submit.waitSemaphoreCount = 1;
		submit.pSignalSemaphores = nullptr;
		submit.signalSemaphoreCount = 0;
		vkQueueSubmit(_queue, 1, &submit, completeFence );
		submitted = true;
	}

	void ScreenCapture::completeCapture()
	{
		memcpy(raw, stagingBuffer->m_raw, stagingBuffer->size());
		capture->onCapture(raw, realLength);
		if( callback )
			callback(capture);
	}

	void ScreenCapture::reset()
	{
		if (stagingBuffer->m_raw) {
			stagingBuffer->unmap();
		}
		delete stagingBuffer;
		stagingBuffer = nullptr;
		texture = nullptr;
		raw = nullptr;
		length = 0;
		capture = nullptr;
		callback = nullptr;
		submitted = false;
	}

}