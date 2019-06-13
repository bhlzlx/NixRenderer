#include "ContextVk.h"
#include "DriverVk.h"
#include "BufferVk.h"
#include "vkhelper/helper.h"
#include "TypemappingVk.h"
#include "RenderPassVk.h"
#include "UniformVk.h"
#include "QueueVk.h"
#include "DeferredDeletor.h"
//#include <glslang/Public/ShaderLang.h>
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

namespace Nix {

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

	Nix::IContext* DriverVk::createContext( void* _hwnd ){
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
		context->m_driver = this;
		context->m_physicalDevice = this->m_PhDevice;

		context->m_swapchain = SwapchainVk::CreateSwapchain(context);
		context->m_renderPass = context->m_swapchain.renderPass();

		context->m_graphicsQueue = context->createGraphicsQueue(graphicsQueue, graphicQueueReq.family, graphicQueueReq.index);
		context->m_uploadQueue = context->createUploadQueue(uploadQueue, uploadQueueReq.family, uploadQueueReq.index);

		context->m_swapchain.setGraphicsQueue((const VkQueue&)* context->m_graphicsQueue);
		//
		VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.physicalDevice = m_PhDevice;
		allocatorInfo.device = context->m_logicalDevice;
		rst = vmaCreateAllocator(&allocatorInfo, &context->m_vmaAllocator);
		if (rst != VK_SUCCESS)
			return nullptr;
		//DVBOVk::Initialize();
		context->m_uniformAllocator.initialize( context );
		context->m_argumentAllocator.initialize(context);
		// Initialize glslang library.
		//glslang::InitializeProcess();
		//
		// read pipeline cache from disk
		IFile* pcFile = m_archieve->open( std::string(PIPELINE_CACHE_FILENAME) );
		VkPipelineCacheCreateInfo cacheInfo = {};
		cacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
		cacheInfo.pNext = NULL;
		cacheInfo.initialDataSize = 0;
		cacheInfo.pInitialData = nullptr;
		cacheInfo.flags = 0;
		if (pcFile) {
			IFile* pcMem = CreateMemoryBuffer(pcFile->size());
			pcFile->read(pcFile->size(), pcMem);
			if (this->validatePipelineCache(pcMem->constData(), pcMem->size())) {
				cacheInfo.pInitialData = pcMem->constData();
				cacheInfo.initialDataSize = pcMem->size();
			}
			pcFile->release();
			pcMem->release();
		}
		vkCreatePipelineCache( context->m_logicalDevice, &cacheInfo, nullptr, &context->m_pipelineCache);
		//
		return context;
	}

	Nix::NixFormat ContextVk::swapchainFormat() const
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

	Nix::UploadQueueVk* ContextVk::getUploadQueue() const
	{
		return m_uploadQueue;
	}

	Nix::GraphicsQueueVk* ContextVk::getGraphicsQueue() const
	{
		return m_graphicsQueue;
	}

	void ContextVk::release()
	{
		vkDeviceWaitIdle( m_logicalDevice );
		savePipelineCache();
		//
		m_renderPass->release();
		m_graphicsQueue->release();
		delete m_uploadQueue;
		m_swapchain.cleanup();
		m_argumentAllocator.cleanup();
		m_uniformAllocator.cleanup();
		//
		delete this;
	}

	bool ContextVk::resume(void* _wnd, uint32_t _width, uint32_t _height )
	{
        m_tickMutex.lock();

        if( m_surface ) {
            return true;
        }
		m_surface = m_driver->createSurface(_wnd);
		if (!m_surface) {
			return false;
		}
		m_swapchain.resize(_width, _height);

        m_tickMutex.unlock();
		return true;
	}

	bool ContextVk::suspend()
	{
		vkQueueWaitIdle((const VkQueue &)*m_graphicsQueue);
		//
		vkDestroySurfaceKHR(m_driver->getInstance(), m_surface, nullptr);
		m_surface = VK_NULL_HANDLE;
		return true;
	}

	Nix::SwapchainVk* ContextVk::getSwapchain()
	{
		return &m_swapchain;
	}

	VkSampler ContextVk::getSampler(const SamplerState& _samplerState)
	{
		union {
			struct alignas(1) {
				uint8_t AddressU;
				uint8_t AddressV;
				uint8_t AddressW;
				uint8_t MinFilter;
				uint8_t MagFilter;
				uint8_t MipFilter;
				uint8_t TexCompareMode;
				uint8_t TexCompareFunc;
			} u;
			SamplerState k;
		} key = {
			static_cast<uint8_t>(_samplerState.u),
			static_cast<uint8_t>(_samplerState.v),
			static_cast<uint8_t>(_samplerState.w),
			static_cast<uint8_t>(_samplerState.min),
			static_cast<uint8_t>(_samplerState.mag),
			static_cast<uint8_t>(_samplerState.mip),
			static_cast<uint8_t>(_samplerState.compareMode),
			static_cast<uint8_t>(_samplerState.compareFunction)
		};
		//SamplerMapMutex.lock();
		// find a sampler by `key`
		auto rst = m_samplerMapping.find(key.k);
		if (rst != m_samplerMapping.end()) {
			//SamplerMapMutex.unlock();
			return rst->second;
		}
		// cannot find a `sampler`, create a new one
		auto sampler = createSampler(_samplerState);
		m_samplerMapping[key.k] = sampler; // insert the sampler to the map
		//SamplerMapMutex.unlock();
		return sampler;
	}

	const std::vector<uint32_t>& ContextVk::getQueueFamilies() const
	{
		return m_queueFamilies;
	}

	void ContextVk::savePipelineCache()
	{
		size_t dataSize;
		vkGetPipelineCacheData(m_logicalDevice, m_pipelineCache, &dataSize, nullptr);
		if (dataSize) {
			void * cache = malloc(dataSize);
			vkGetPipelineCacheData(m_logicalDevice, m_pipelineCache, &dataSize, cache);
			m_driver->getArchieve()->save(std::string(PIPELINE_CACHE_FILENAME), cache, dataSize);
		}
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
			info.addressModeU = NixAddressModeToVk(_ss.u);
			info.addressModeV = NixAddressModeToVk(_ss.v);
			info.addressModeW = NixAddressModeToVk(_ss.w);
			info.compareOp = NixCompareOpToVk(_ss.compareFunction);
			info.compareEnable = _ss.compareMode != RefNone;
			info.magFilter = NixFilterToVk(_ss.mag);
			info.minFilter = NixFilterToVk(_ss.min);
			info.mipmapMode = NixMipmapFilerToVk(_ss.mip);
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
	    m_tickMutex.lock();
	    if( m_surface ) {
		    m_swapchain.resize(_width, _height);
	    }
	    m_tickMutex.unlock();
	}

	bool ContextVk::beginFrame()
	{
        m_tickMutex.lock();

		if (!m_surface) {
            m_tickMutex.unlock();
			return false;
		}
		if (!m_swapchain.acquireNextImage()){
            m_tickMutex.unlock();
			return false;
        }
		m_graphicsQueue->beginFrame( m_swapchain.getFlightIndex());
		GetDeferredDeletor().tick( m_swapchain.getFlightIndex());
		++m_frameCounter;
		return true;
	}

	void ContextVk::endFrame()
	{
		m_graphicsQueue->endFrame();
		m_swapchain.present();
        //
        m_tickMutex.unlock();
	}

	IGraphicsQueue* ContextVk::getGraphicsQueue(uint32_t _index) {
		return m_graphicsQueue;
	}
}