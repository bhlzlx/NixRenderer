#include "ViewVk.h"
#include "BufferVk.h"
#include "SwapchainVk.h"
#include "ContextVk.h"
#include "RenderPassVk.h"
#include "QueueVk.h"
#include "DeferredDeletor.h"

#include <exception>

namespace Ks {

	IView* ContextVk::createView(void* _nativeWindow) {
		ViewVk* view = new ViewVk();
		view->m_swapchain = &m_swapchain;
		view->m_context = this;
		view->m_graphicsQueue = m_graphicsQueue;
		view->m_renderPass = m_swapchain.renderPass();
		return view;
	}

	void ViewVk::resize(uint32_t _width, uint32_t _height)
	{
		m_swapchain->resize(_width, _height);
	}

	bool ViewVk::beginFrame()
	{
		if (!m_swapchain)
			return false;
		//
		if (!m_swapchain->acquireNextImage())
			return false;
		m_flightIndex = m_swapchain->getFlightIndex();
		m_graphicsQueue->beginFrame(m_flightIndex);
		GetDeferredDeletor().tick(m_flightIndex);
		++m_context->m_frameCounter;
		//DVBOVk::Begin(m_flightIndex);
		//m_state = DeviceState::onActive;
		return true;
	}

	void ViewVk::endFrame()
	{
		//DVBOVk::End();
		m_graphicsQueue->endFrame();
		m_swapchain->present();
	}

	IRenderPass* ViewVk::getRenderPass()
	{
		return m_renderPass;
	}

	IContext* ViewVk::getContext()
	{
		return m_context;
	}

	void ViewVk::release()
	{
		m_graphicsQueue->waitForIdle();
		// todo...
		// destroy `VkSampler` `VkRenderPass` `VkShaderModules`
		delete this;
	}
}


