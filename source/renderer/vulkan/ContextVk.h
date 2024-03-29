﻿#pragma once
#include <NixRenderer.h>
#include "VkInc.h"
#include "SwapchainVk.h"
#include "DescriptorSetVk.h"
#include "RingBuffer.h"
#include "DebuggerVk.h"
#include "vk_mem_alloc.h"
#include <nix/io/archieve.h>
#include <vector>
#include <mutex>

#define PIPELINE_CACHE_FILENAME "pipeline_cache.bin"

namespace Nix {
	class GraphicsQueueVk;
	class UploadQueueVk;
	class BufferVk;
	class ShaderModuleVk;
	class SwapchainVk;
	class BufferAllocatorVk;
	class DriverVk;
	// a context owns a phy/logic device graphics queue,present queue
	// contexts shares a upload queue
	class NIX_API_DECL ContextVk : public IContext {
		friend class DriverVk;
	private:
		std::mutex              m_tickMutex;
		DriverVk* m_driver;
		//
		VkPhysicalDevice		m_physicalDevice;
		VkDevice				m_logicalDevice;
		// Queues
		std::vector<uint32_t>	m_queueFamilies;
		GraphicsQueueVk*		m_graphicsQueue;
		UploadQueueVk*			m_uploadQueue;
		// surface & swapchain
		VkSurfaceKHR			m_surface;
		SwapchainVk				m_swapchain;
		IRenderPass*			m_renderPass;
		//
		VmaAllocator			m_vmaAllocator;
		ArgumentAllocator		m_argumentAllocator;
		IArchieve*				m_archieve;
		//
		uint64_t				m_frameCounter;

		BufferAllocatorVk*		m_staticBufferAllocator;
		BufferAllocatorVk*		m_uniformBufferAllocator;
		BufferAllocatorVk*		m_stagingBufferAllocator;
	private:
		VkPipelineCache			m_pipelineCache;
		std::map< SamplerState, VkSampler > m_samplerMapping;
		VkSampler createSampler(const SamplerState& _ss);
	public:
		ContextVk() :
			m_physicalDevice(VK_NULL_HANDLE),
			m_logicalDevice(VK_NULL_HANDLE),
			m_archieve(nullptr),
			m_frameCounter(0)
		{
		}

		~ContextVk() {

		}

		virtual IBuffer* createVertexBuffer(const void* _data, size_t _size) override;
		virtual IBuffer* createIndexBuffer(const void* _data, size_t _size) override;
		virtual IBuffer* createTexelBuffer(size_t _size) override;
		virtual IBuffer* createStorageBuffer(size_t _size) override;

		virtual IBuffer* createUniformBuffer(size_t _size) override;

		BufferVk* createStagingBuffer(size_t _size);



		// staging buffer is an inner buffer type
		//virtual IBuffer* createStagingBuffer( size_t _size );

		virtual ITexture* createTexture(const TextureDescription& _desc, TextureUsageFlags _usage = TextureUsageNone) override;
		virtual ITexture* createTextureDDS(const void* _data, size_t _length) override;
		virtual ITexture* createTextureKTX(const void* _data, size_t _length) override;
		virtual IAttachment* createAttachment(NixFormat _format, uint32_t _width, uint32_t _height) override;
		virtual IRenderPass* createRenderPass(const RenderPassDescription& _desc, IAttachment** _colorAttachments, IAttachment* _depthStencil) override;
		virtual IMaterial* createMaterial(const MaterialDescription& _desc);
		virtual NixFormat swapchainColorFormat() const override;
		virtual NixFormat swapchainDepthFormat() const override;
		//virtual void captureFrame(IFrameCapture * _capture, FrameCaptureCallback _callback) override;
		//
		VkSurfaceKHR getSurface() const;
		VkDevice getDevice() const;
		VkPhysicalDevice getPhysicalDevice() const;
		VkInstance getInstance() const;
		UploadQueueVk* getUploadQueue() const;
		GraphicsQueueVk* getGraphicsQueue() const;
		SwapchainVk* getSwapchain();
		//
		VkSampler getSampler(const SamplerState& _samplerState);
		//
		const std::vector<uint32_t>& getQueueFamilies() const;

		VkPipelineCache getPipelineCache() { return m_pipelineCache; }
		void savePipelineCache();

		inline ArgumentAllocator& getDescriptorSetPool() { return m_argumentAllocator; }
		inline VmaAllocator getVmaAllocator() { return m_vmaAllocator; }
		inline uint64_t getFrameCounter() const { return m_frameCounter; }
		//
		VkSemaphore createSemaphore() const;
		VkFence createFence() const;
		GraphicsQueueVk* createGraphicsQueue(VkQueue _id, uint32_t _family, uint32_t _index) const;
		UploadQueueVk* createUploadQueue(VkQueue _id, uint32_t _family, uint32_t _index) const;
		//ShaderModuleVk* createShaderModule(const char * _text, const char * _entryPoint, VkShaderStageFlagBits _stage) const;
		//BufferVk&& createBuffer( size_t _size, VkBufferUsageFlags _usage ) const;
		virtual void resize(uint32_t _width, uint32_t _height) override;
		virtual bool beginFrame() override;
		virtual void executeCompute(const ComputeCommand& _command) override;
		virtual void endFrame() override;
		virtual inline IDriver* getDriver() override { return (IDriver*)m_driver; }
		virtual IGraphicsQueue* getGraphicsQueue(uint32_t _index) override;
		virtual inline IRenderPass* getRenderPass() override { return m_swapchain.renderPass(); }
		virtual void release() override;
		virtual uint32_t getMaxFlightCount() override { return MaxFlightCount; }
		// for android platform
		virtual bool resume(void* _wnd, uint32_t _width, uint32_t _height) override;
		virtual bool suspend() override;

	};
}
