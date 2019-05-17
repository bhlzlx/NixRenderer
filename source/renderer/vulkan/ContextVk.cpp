#include "ContextVk.h"
#include "DriverVk.h"
#include "BufferVk.h"
#include "vkhelper/helper.h"
#include "TypemappingVk.h"
#include "RenderPassVk.h"
#include "UniformVk.h"
#include "QueueVk.h"
#include "DeferredDeletor.h"
#include <glslang/Public/ShaderLang.h>
#ifdef _MINWINDEF_
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#endif
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

namespace Ks {

	std::vector<VkDeviceQueueCreateInfo> createDeviceQueueCreateInfo(std::vector< QueueReqData > _queues) {

		std::map< uint32_t, uint32_t > records;
		for (auto& req : _queues)
		{
			uint32_t family = req.family;
			if (records.find(family) == records.end())
			{
				records[family] = 1;
			}
			else
			{
				++records[family];
			}
		}
		std::vector<VkDeviceQueueCreateInfo> createInfos;

		const static float Priorities[] = {
			1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f
		};

		for (auto& record : records)
		{
			VkDeviceQueueCreateInfo info;
			info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			info.pNext = nullptr;
			info.flags = 0;
			info.pQueuePriorities = &Priorities[0];
			info.queueCount = record.second;
			info.queueFamilyIndex = record.first;
			createInfos.push_back(info);
		}
		return createInfos;
	}

	Ks::IContext* DriverVk::createContext( void* _hwnd ){
		VkResult rst = VK_SUCCESS;
		//
		ContextVk* context = new ContextVk();
		context->m_archieve = m_archieve;
		context->m_surface = this->createSurface(_hwnd);
		QueueReqData graphicQueueReq, uploadQueueReq;
		graphicQueueReq.id = this->requestGraphicsQueue(context->m_surface);
		uploadQueueReq.id = this->requestTransferQueue();
		std::vector<QueueReqData> requestedQueues = {
			graphicQueueReq, uploadQueueReq
		};
		auto queueInfo = createDeviceQueueCreateInfo(requestedQueues);
		context->m_logicalDevice = createDevice(queueInfo);
		VkQueue graphicsQueue, uploadQueue;
		vkGetDeviceQueue( context->m_logicalDevice, graphicQueueReq.family, graphicQueueReq.index, &graphicsQueue);
		vkGetDeviceQueue( context->m_logicalDevice, uploadQueueReq.family, uploadQueueReq.index, &uploadQueue);
		// queue families
		for (auto& req : requestedQueues) {
			auto queueFamily = req.family;
			if (std::find( context->m_queueFamilies.begin(), context->m_queueFamilies.end(), queueFamily) == context->m_queueFamilies.end()) {
				context->m_queueFamilies.push_back(queueFamily);
			}
		}
		context->m_swapchain = SwapchainVk::CreateSwapchain();
		context->m_renderPass = context->m_swapchain.renderPass();
		context->m_graphicsQueue = context->createGraphicsQueue(graphicsQueue, graphicQueueReq.family, graphicQueueReq.index );
		context->m_uploadQueue = context->createUploadQueue(uploadQueue, uploadQueueReq.family, uploadQueueReq.index );
		context->m_swapchain.setGraphicsQueue((const VkQueue&)* context->m_graphicsQueue);
		//
		VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.physicalDevice = m_device;
		allocatorInfo.device = context->m_logicalDevice;
		rst = vmaCreateAllocator(&allocatorInfo, &KsVMAAllocator);
		if (rst != VK_SUCCESS)
			return false;
		//DVBOVk::Initialize();
		context->m_uboAllocator.initialize();
		// Initialize glslang library.
		glslang::InitializeProcess();
		//
		return context;
	}

	Ks::KsFormat ContextVk::swapchainFormat() const
	{
		return m_swapchain.format();
	}
	/////////////////////////////////

	VkSurfaceKHR ContextVk::getSurface() const {
		return m_surface;
	}
	VkDevice ContextVk::getDevice() const {
		return m_logicalDevice;
	}

	VkPhysicalDevice ContextVk::getPhysicalDevice() const
	{
		return m_physicalDevice;
	}

	VkInstance ContextVk::getInstance() const {
		return m_driver->getInstance();
	}

	Ks::UploadQueueVk* ContextVk::getUploadQueue() const
	{
		return m_uploadQueue;
	}

	Ks::GraphicsQueueVk* ContextVk::getGraphicsQueue() const
	{
		return m_graphicsQueue;
	}

	Ks::SwapchainVk* ContextVk::getSwapchain()
	{
		return &m_swapchain;
	}

	const std::vector<uint32_t>& ContextVk::getQueueFamilies() const
	{
		return m_queueFamilies;
	}

	VkSemaphore ContextVk::createSemaphore() const
	{
		VkSemaphoreCreateInfo semaphoreInfo; {
			semaphoreInfo.flags = 0;
			semaphoreInfo.pNext = nullptr;
			semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		}
		VkSemaphore semaphore = VK_NULL_HANDLE;
		vkCreateSemaphore(m_logicalDevice, &semaphoreInfo, nullptr, &semaphore);
		return semaphore;
	}

	VkFence ContextVk::createFence() const
	{
		VkFenceCreateInfo fenceInfo; {
			fenceInfo.flags = 0;
			fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fenceInfo.pNext = nullptr;
		}
		VkFence fence;
		vkCreateFence(m_logicalDevice, &fenceInfo, nullptr, &fence);
		return fence;
	}

	VkSampler ContextVk::createSampler(const SamplerState& _ss) {
		VkSamplerCreateInfo info;
		{
			info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			info.flags = 0;
			info.pNext = nullptr;
			info.addressModeU = KsAddressModeToVk(_ss.u);
			info.addressModeV = KsAddressModeToVk(_ss.v);
			info.addressModeW = KsAddressModeToVk(_ss.w);
			info.compareOp = KsCompareOpToVk(_ss.compareFunction);
			info.compareEnable = _ss.compareMode != RefNone;
			info.magFilter = KsFilterToVk(_ss.mag);
			info.minFilter = KsFilterToVk(_ss.min);
			info.mipmapMode = KsMipmapFilerToVk(_ss.mip);
			info.borderColor = VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
			info.anisotropyEnable = VK_FALSE;
			info.mipLodBias = 0;
			info.maxAnisotropy = 0;
			info.minLod = 0;
			info.maxLod = 0;
			info.unnormalizedCoordinates = 0;
		}
		VkSampler sampler;
		vkCreateSampler( m_logicalDevice, &info, nullptr, &sampler);
		return sampler;
	}

	void ContextVk::captureFrame(IFrameCapture * _capture, FrameCaptureCallback _callback) {
		RenderPassSwapchainVk* rp = (RenderPassSwapchainVk*)m_swapchain.renderPass();
		TextureVk* texture = rp->colorAttachment(m_swapchain.getFlightIndex());
		m_graphicsQueue->captureScreen(texture, _capture->rawData(), _capture->rawSize(), _callback, _capture);
	}

	void ContextVk::resize(uint32_t _width, uint32_t _height)
	{
		m_swapchain.resize(_width, _height);
	}

	bool ContextVk::beginFrame()
	{
		if (!m_swapchain.acquireNextImage())
			return false;
		m_graphicsQueue->beginFrame( m_swapchain.getFlightIndex());
		GetDeferredDeletor().tick( m_swapchain.getFlightIndex());
		++m_frameCounter;
		return true;
	}

	void ContextVk::endFrame()
	{
		m_graphicsQueue->endFrame();
		m_swapchain.present();
	}
}
