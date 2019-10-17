#pragma once
#include <NixRenderer.h>
#include "VkInc.h"
#include <map>

namespace Nix {

	class PipelineVk;
	class TextureVk;
	class ArgumentVk;
	class ShaderModuleVk;
	class ContextVk;
	class RenderPassVk;

	class NIX_API_DECL PipelineVk : public IPipeline
	{
		friend class GraphicsQueueAsyncTaskManager;
		friend class ContextVk;
		friend class MaterialVk;
	private:
		ContextVk*		m_context;
		VkPipeline		m_pipeline;
		VkRenderPass	m_renderPass;
		//VkViewport		m_viewport;
		//VkCommandBuffer m_commandBuffer;
		//VkRect2D		m_scissor;
		//
		float			m_blendConstants[4];
		float			m_constantBias;
		float			m_slopeScaleBias;
		uint32_t		m_stencilReference;
		//
		//RenderPassVk*	m_renderPass;
	public:
		//virtual void setViewport(const Viewport& _viewport) override;
		//virtual void setScissor(const Scissor& _scissor) override;
		virtual void setPolygonOffset(float _constantBias, float _slopeScaleBias) override;
		virtual void release() override;
	public:
		PipelineVk();
		~PipelineVk();
		//
		operator VkPipeline () const {
			return m_pipeline;
		}
		//
		void setStencilReference(uint32_t _stencilReference);
		void setBlendFactor(const float* _blendFactors);
		void setDynamicalStates(VkCommandBuffer _commandBuffer);
	public:

	};
}
