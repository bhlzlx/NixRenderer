#pragma once
#include <NixRenderer.h>
#include "vkinc.h"
#include <map>

namespace nix {

	class PipelineVk;
	class TextureVk;
	class ArgumentVk;
	class ShaderModuleVk;
	class ContextVk;
	//

	class NIX_API_DECL PipelineVk : public IPipeline
	{
		friend class GraphicsQueueAsyncTaskManager;
		friend class ContextVk;
	private:
		ContextVk* m_context;
		VkRenderPass m_renderPass;
		VkViewport m_viewport;
		VkRect2D m_scissor;
		//
		float m_blendConstants[4];
		uint32_t m_stencilReference;
		float m_constantBias;
		float m_slopeScaleBias;
		//
		static VkPipelineCache GeneralPipelineCache;
	public:
		virtual void setViewport(const Viewport& _viewport) override;
		virtual void setScissor(const Scissor& _scissor) override;
		virtual void setPolygonOffset(float _constantBias, float _slopeScaleBias) override;
		virtual void release() override;
	public:
		PipelineVk() :
			m_renderPass(VK_NULL_HANDLE),
			m_stencilReference(0),
			m_constantBias(0),
			m_slopeScaleBias(0)
		{
			m_blendConstants[0] = m_blendConstants[1] = m_blendConstants[2] = m_blendConstants[3] = 1.0f;
		}
		~PipelineVk();
		//
		void setStencilReference( uint32_t _stencilReference );
		void setBlendFactor(const float* _blendFactors);
	public:

	};
}