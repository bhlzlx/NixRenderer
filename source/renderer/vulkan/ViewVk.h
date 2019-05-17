#pragma once
#include <KsRenderer.h>

namespace Ks {

	class ContextVk;
	class SwapchainVk;
	class GraphicsQueueVk;

	class KS_API_DECL ViewVk : public IView {
		friend class ContextVk;
	private:
		ContextVk* m_context;
		SwapchainVk* m_swapchain;
		GraphicsQueueVk* m_graphicsQueue;
		IRenderPass* m_renderPass; // main render pass!
		uint32_t m_flightIndex;
	public:
		ViewVk() : m_context(nullptr), m_swapchain(nullptr) {
		}
		virtual void resize(uint32_t _width, uint32_t _height) override;
		virtual bool beginFrame() override;
		virtual void endFrame() override;
		virtual IRenderPass* getRenderPass() override;
		virtual IContext* getContext() override;
		virtual void release() override;
	};

}