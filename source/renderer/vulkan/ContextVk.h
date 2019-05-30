#pragma once
#include <NixRenderer.h>
#include "UniformVk.h"
#include "vkinc.h"
#include "SwapchainVk.h"
#include "DescriptorSetVk.h"
#include "DebuggerVk.h"
#include <nix/io/archieve.h>
#include <vector>

#define PIPELINE_CACHE_FILENAME "pipeline_cache.bin"

// create sampler must be a thread safe function
// engine may be create resources in a background thread
// std::map< SamplerState, VkSampler > SamplerMapVk;
// std::mutex SamplerMapMutex;
// VkSampler GetSampler(const SamplerState& _state)
// {
// 	// construct a `key`
// 	union {
// 		struct alignas(1) {
// 			uint8_t AddressU;
// 			uint8_t AddressV;
// 			uint8_t AddressW;
// 			uint8_t MinFilter;
// 			uint8_t MagFilter;
// 			uint8_t MipFilter;
// 			uint8_t TexCompareMode;
// 			uint8_t TexCompareFunc;
// 		} u;
// 		SamplerState k;
// 	} key = {
// 		static_cast<uint8_t>(_state.u),
// 		static_cast<uint8_t>(_state.v),
// 		static_cast<uint8_t>(_state.w),
// 		static_cast<uint8_t>(_state.min),
// 		static_cast<uint8_t>(_state.mag),
// 		static_cast<uint8_t>(_state.mip),
// 		static_cast<uint8_t>(_state.compareMode),
// 		static_cast<uint8_t>(_state.compareFunction)
// 	};
// 	SamplerMapMutex.lock();
// 	// find a sampler by `key`
// 	auto rst = SamplerMapVk.find(key.k);
// 	if (rst != SamplerMapVk.end()) {
// 		SamplerMapMutex.unlock();
// 		return rst->second;
// 	}
// 	// cannot find a `sampler`, create a new one
// 	auto sampler = m_context->createSampler(_state);
// 	SamplerMapVk[key.k] = sampler; // insert the sampler to the map
// 	SamplerMapMutex.unlock();
// 	return sampler;
// }

namespace nix {
	class GraphicsQueueVk;
	class UploadQueueVk;
	class BufferVk;
	class ShaderModuleVk;
	class SwapchainVk;
	class DriverVk;
	// a context owns a phy/logic device graphics queue,present queue
	// contexts shares a upload queue
	class NIX_API_DECL ContextVk : public IContext {
		//friend class ViewVk;
		friend class DriverVk;
	private:
		DriverVk* m_driver;
		//
		VkPhysicalDevice		m_physicalDevice;
		VkDevice				m_logicalDevice;
		// Queues
		uint32_t				m_graphicQueueID;
		uint32_t				m_transferQueueID;
		std::vector<uint32_t>	m_queueFamilies;
		GraphicsQueueVk*		m_graphicsQueue;
		UploadQueueVk*			m_uploadQueue;
		// surface & swapchain
		VkSurfaceKHR			m_surface;
		SwapchainVk				m_swapchain;
		IRenderPass*			m_renderPass;
		// buffer object manager
		UniformAllocator		m_uniformAllocator;
		// descriptor set manager
		ArgumentAllocator		m_descriptorSetPool;
		//
		IArchieve*				m_archieve;
		//
		uint64_t				m_frameCounter;
		//
		VkCommandBuffer			m_renderCommandBuffer;
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

		virtual IBuffer* createStableVBO(const void* _data, size_t _size) override;
		virtual IBuffer* createTransientVBO(size_t _size) override;
		virtual IBuffer* createIndexBuffer(const void* _data, size_t _size) override;
		virtual ITexture* createTexture(const TextureDescription& _desc, TextureUsageFlags _usage = TextureUsageNone) override;
		virtual ITexture* createTextureDDS(const void* _data, size_t _length) override;
		virtual ITexture* createTextureKTX(const void* _data, size_t _length) override;
		virtual IAttachment* createAttachment( NixFormat _format, uint32_t _width, uint32_t _height ) override;
		virtual IRenderPass* createRenderPass(const RenderPassDescription& _desc, IAttachment** _colorAttachments, IAttachment* _depthStencil) override;
		virtual IMaterial* createMaterial( const MaterialDescription& _desc );
		virtual NixFormat swapchainFormat() const override;
		virtual void captureFrame(IFrameCapture * _capture, FrameCaptureCallback _callback) override;
		//
		VkSurfaceKHR getSurface() const;
		VkDevice getDevice() const;
		VkPhysicalDevice getPhysicalDevice() const;
		VkInstance getInstance() const;
		UploadQueueVk* getUploadQueue() const;
		GraphicsQueueVk* getGraphicsQueue() const;
		SwapchainVk* getSwapchain();
		//
		VkSampler getSampler( const SamplerState& _samplerState );
		//
		const std::vector<uint32_t>& getQueueFamilies() const;

		VkPipelineCache getPipelineCache() { return m_pipelineCache; }
		void savePipelineCache();

		inline UniformAllocator& getUniformAllocator(){ return m_uniformAllocator; }
		inline ArgumentAllocator& getDescriptorSetPool(){ return m_descriptorSetPool; }
		inline uint64_t getFrameCounter() const { return m_frameCounter; }
		//
		VkSemaphore createSemaphore() const;
		VkFence createFence() const;
		GraphicsQueueVk* createGraphicsQueue(VkQueue _id, uint32_t _family, uint32_t _index ) const;
		UploadQueueVk* createUploadQueue(VkQueue _id, uint32_t _family, uint32_t _index ) const;
		ShaderModuleVk* createShaderModule(const char * _text, const char * _entryPoint, VkShaderStageFlagBits _stage) const;		
		//BufferVk&& createBuffer( size_t _size, VkBufferUsageFlags _usage ) const;
		virtual void resize(uint32_t _width, uint32_t _height) override;
		virtual bool beginFrame() override;
		virtual void endFrame() override;
		virtual inline IDriver* getDriver() override { return (IDriver*)m_driver; }
		virtual IGraphicsQueue* getGraphicsQueue(uint32_t _index) override;
		virtual inline IRenderPass* getRenderPass() override { return m_swapchain.renderPass(); }
	};
}