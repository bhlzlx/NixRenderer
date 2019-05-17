#pragma once
#include <NixRenderer.h>
#include "vkinc.h"
#include <vector>

namespace Ks {

	class ContextVk;
	class RenderPassVk;
	class TextureVk;
	class AttachmentVk;

	class KS_API_DECL SwapchainVk {
	private:
		VkSwapchainCreateInfoKHR m_createInfo;
		VkSwapchainKHR m_swapchain;
		// resource objects
		VkSemaphore m_nextImageAvailable;
		VkSemaphore m_readyToPresent;
		//
		//std::vector< VkImage > m_vecImages;
		IRenderPass* m_renderPass;
		/*
		std::vector< TextureVk* > m_vecTextures;
		TextureVk* m_depthStencil;
		std::vector< AttachmentVk* > m_vecAttachmentVk;
		AttachmentVk* m_depthStencilAttachment;
		std::vector< RenderPassVk* > m_vecRenderPass;
		*/
		//
		VkCommandBuffer m_commandBuffer;
		VkQueue m_graphicsQueue;
		// state description
		uint32_t m_imageIndex;
		uint32_t m_flightIndex;
		VkBool32 m_available;
		Size<uint32_t> m_size;
	public:
		SwapchainVk() :
			m_nextImageAvailable( VK_NULL_HANDLE)
			, m_readyToPresent(VK_NULL_HANDLE)
			, m_renderPass(nullptr)
			, m_commandBuffer(VK_NULL_HANDLE)
			, m_graphicsQueue(VK_NULL_HANDLE)
			, m_imageIndex(0)
			, m_flightIndex(0)
			, m_available( VK_FALSE)
		{

		}
		void resize( uint32_t _width, uint32_t _height );
		void cleanup();

		const VkSemaphore& getImageAvailSemaphore() const {
			return m_nextImageAvailable;
		}
		const VkSemaphore& getReadyToPresentSemaphore() const {
			return m_readyToPresent;
		}
		uint32_t getFlightIndex() const {
			return m_flightIndex;
		}

		VkFormat nativeFormat() const;
		KsFormat format() const;
		VkBool32 avail() const;
		//
		bool updateSwapchain();
		bool acquireNextImage();

		void setGraphicsQueue(VkQueue _queue) {
			m_graphicsQueue = _queue;
		}

		IRenderPass* renderPass() {
			return m_renderPass;
		}
		bool present();

	public:
		static SwapchainVk CreateSwapchain();

	};
}