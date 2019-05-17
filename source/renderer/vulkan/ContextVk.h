#pragma once
#include <KsRenderer.h>
#include "UniformVk.h"
#include "vkinc.h"
#include "SwapchainVk.h"
#include "DescriptorSetVk.h"
#include "DebuggerVk.h"
#include <Ks/io/io.h>
#include <vector>

namespace Ks {
	class GraphicsQueueVk;
	class UploadQueueVk;
	class BufferVk;
	class ShaderModuleVk;
	class SwapchainVk;
	class DriverVk;
	// a context owns a phy/logic device graphics queue,present queue
	// contexts shares a upload queue
	class KS_API_DECL ContextVk : public IContext {
		//friend class ViewVk;
		friend class DriverVk;
	private:
		DriverVk* m_driver;
		//
		VkPhysicalDevice	m_physicalDevice;
		VkDevice			m_logicalDevice;
		// Queues
		uint32_t				m_graphicQueueID;
			uint32_t			m_transferQueueID;
		std::vector<uint32_t>	m_queueFamilies;
		GraphicsQueueVk*		m_graphicsQueue;
		UploadQueueVk*			m_uploadQueue;
		// surface & swapchain
		VkSurfaceKHR			m_surface;
		SwapchainVk				m_swapchain;
		IRenderPass*			m_renderPass;
		// buffer object manager
		UBOAllocator			m_uboAllocator;
		// descriptor set manager
		DescriptorSetPool		m_descriptorSetPool;
		//
		IArchieve*				m_archieve;
		//
		uint64_t				m_frameCounter;
		//
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
		virtual IAttachment* createAttachment(KsFormat _format, uint32_t _width, uint32_t _height ) override;
		virtual IRenderPass* createRenderPass(const RenderPassDescription& _desc, IAttachment** _colorAttachments, IAttachment* _depthStencil) override;
		virtual IPipeline* createPipeline(const PipelineDescription& _desc) override;
		virtual KsFormat swapchainFormat() const override;
		virtual void captureFrame(IFrameCapture * _capture, FrameCaptureCallback _callback) override;
		//
		VkSurfaceKHR getSurface() const;
		VkDevice getDevice() const;
		VkPhysicalDevice getPhysicalDevice() const;
		VkInstance getInstance() const;
		UploadQueueVk* getUploadQueue() const;
		GraphicsQueueVk* getGraphicsQueue() const;
		SwapchainVk* getSwapchain();
		const std::vector<uint32_t>& getQueueFamilies() const;

		inline UBOAllocator& getUBOAllocator(){ return m_uboAllocator;	}
		inline DescriptorSetPool& getDescriptorSetPool(){ return m_descriptorSetPool; }
		inline uint64_t getFrameCounter() const { return m_frameCounter; }
		//
		VkSemaphore createSemaphore() const;
		VkFence createFence() const;
		VkSampler createSampler( const SamplerState& _ss );
		GraphicsQueueVk* createGraphicsQueue(VkQueue _id, uint32_t _family, uint32_t _index ) const;
		UploadQueueVk* createUploadQueue(VkQueue _id, uint32_t _family, uint32_t _index ) const;
		ShaderModuleVk* createShaderModule(const char * _text, const char * _entryPoint, VkShaderStageFlagBits _stage) const;		
		//BufferVk&& createBuffer( size_t _size, VkBufferUsageFlags _usage ) const;
		virtual void resize(uint32_t _width, uint32_t _height) override;
		virtual bool beginFrame() override;
		virtual void endFrame() override;
		virtual inline IDriver* getDriver() override { return (IDriver*)m_driver; }
		virtual inline IRenderPass* getRenderPass() override { return m_swapchain.renderPass(); }
	};
}