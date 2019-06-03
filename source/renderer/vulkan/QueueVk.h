#pragma once

#include <NixRenderer.h>
#include "vkinc.h"
#include <vector>
#include <thread>
#include <mutex>
#include <cassert>
#include "TypemappingVk.h"

namespace nix {

	class RenderPassVk;
	class SwapchainVk;
	class BufferVk;
	class TextureVk;

	class NIX_API_DECL CommandBufferVk
	{
		friend class ContextVk;
		friend class RenderPassVk;
		friend struct ScreenCapture;
	private:
		ContextVk*		m_contextVk;
		VkCommandBuffer m_commandBuffer;
		RenderPassVk*	m_renderPass;
		//uint32_t		m_subPassIndex;
	public:
		CommandBufferVk() :
			m_commandBuffer(VK_NULL_HANDLE),
			m_renderPass(VK_FALSE)
			//,m_subPassIndex(0)
		{
		}

		bool begin() const;
		void end() const;

		void updateBuffer(BufferVk* _buffer, size_t _offset, size_t _size, const void* _data);
		//
		void updateTexture(TextureVk* _texture, const void* _data, size_t _length, const TextureRegion& _region) const;
		void updateTexture(TextureVk* _texture, const void* _data, size_t _length, const TextureRegion& _baseMipRegion, uint32_t _mipCount ) const;
		void getFramePixels(TextureVk* _texture, BufferVk& _stagingBuffer);
		//
		void setViewport(VkViewport _vp);
		void setScissor(VkRect2D _scissor);
		//
		bool isEncoding() const {
			return m_renderPass != nullptr;
		}
		operator const VkCommandBuffer&() const {
			return m_commandBuffer;
		}
	};
	//
	class AttachmentVk;
	class BufferVk;
	struct ScreenCapture {
		//
		ContextVk*		context;
		TextureVk*		texture;
		void*			raw;
		size_t			length;
		size_t			realLength;
		//
		BufferVk*		stagingBuffer;
		//
		VkFence			completeFence;
		VkSemaphore		waitSemaphore;
		CommandBufferVk commandBuffer;
		//
		uint64_t		invokeFrameCount;
		IFrameCapture*  capture;
		FrameCaptureCallback	callback;

		bool			submitted;

		ScreenCapture();

		void initialize( ContextVk* _context, VkCommandPool _pool, TextureVk* _texture, void* _raw, size_t _length, FrameCaptureCallback _callback, IFrameCapture* _userData );
		void submitCommand( VkQueue _queue );
		void completeCapture();
		void reset();
	};

	class NIX_API_DECL GraphicsQueueVk : public IGraphicsQueue
	{
		friend class ContextVk;
	private:
		ContextVk*					m_context;
		VkQueue						m_queue;
		uint32_t					m_queueFamily;
		uint32_t					m_queueIndex;
		VkCommandPool				m_commandPool; // one command pool per queue/thread
		VkDevice					m_device; // host device
		// rendering buffer and synchronization objects
		VkFence						m_renderFences[MaxFlightCount]; // one fence per frame
		VkBool32					m_renderFencesActived[MaxFlightCount];
		CommandBufferVk				m_renderBuffers[MaxFlightCount];
		// updating buffer and synchronization objects		
		VkFence						m_updatingFences[MaxFlightCount];
		VkBool32					m_updatingFencesActived[MaxFlightCount];
		CommandBufferVk				m_updatingBuffers[MaxFlightCount];
		//
		VkBool32					m_updatingBuffersActived[MaxFlightCount];
		//
		uint32_t					m_flightIndex;
		// semaphores reference to the swapchain
		VkSemaphore					m_imageAvailSemaphore;
		VkSemaphore					m_renderCompleteSemaphore;
		// indicate the graphics queue is ready for rendering
		VkBool32					m_readyForRendering;

		ScreenCapture				m_screenCapture;
	private:
		GraphicsQueueVk(const GraphicsQueueVk&) {
		}
		GraphicsQueueVk(GraphicsQueueVk&&) {
		}
	public:
		GraphicsQueueVk() :
			m_queue(VK_NULL_HANDLE)
			, m_commandPool(VK_NULL_HANDLE)
			, m_flightIndex(0)
			, m_imageAvailSemaphore( VK_NULL_HANDLE)
			, m_renderCompleteSemaphore( VK_NULL_HANDLE)
			, m_readyForRendering( VK_FALSE)
		{
			memset(m_renderFences, 0, sizeof(m_renderFences));
			memset(m_renderFencesActived, 0, sizeof(m_renderFencesActived));
			memset(m_renderBuffers, 0, sizeof(m_renderBuffers));
			memset(m_updatingBuffersActived, 0, sizeof(m_updatingBuffersActived));		}
		//
		operator const VkQueue& () const {
			return m_queue;
		}
		bool intialize(SwapchainVk* _swapchain);
		void beginFrame(uint32_t _flightIndex);
		void updateBuffer( BufferVk* _buffer, size_t _offset, const void * _data, size_t _length );
		//void updateTexture(TextureVk* _texture, const ImageRegion& _region, const void * _data, size_t _length);
		void updateTexture(TextureVk* _texture, const TextureRegion& _region, const void * _data, size_t _length);
		void updateTexture(TextureVk* _texture, const TextureRegion& _region, uint32_t _mipCount, const void * _data, size_t _length);
		void captureScreen(TextureVk* _texture, void * raw_, size_t _length, FrameCaptureCallback _callback, IFrameCapture* _capture);
		const CommandBufferVk* commandBuffer() const;
		void endFrame();
		void waitForIdle() const;

		virtual void release() override;
	};

	class NIX_API_DECL UploadQueueVk
	{
		friend class ContextVk;
	public:
	private:
		ContextVk* m_context;
		//
		VkQueue m_uploadQueue;
		uint32_t m_queueFamily;
		uint32_t m_queueIndex;
		VkCommandPool m_uploadCommandPool;
		CommandBufferVk m_commandBuffer;
		std::mutex m_uploadMutex;
	public:
		UploadQueueVk() {
		}
		//
		void uploadBuffer( BufferVk* _buffer, size_t _offset, size_t _size, const void * _data);
		void uploadTexture(TextureVk* _texture, const void* _data, size_t _length, const TextureRegion& _region);
		~UploadQueueVk();
	};
}