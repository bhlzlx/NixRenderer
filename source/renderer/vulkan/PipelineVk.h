#pragma once
#include <NixRenderer.h>
#include "vkinc.h"
#include "ShaderVk.h"
#include <map>

namespace Ks {

	class PipelineVk;
	class TextureVk;
	class ArgumentVk;
	class ShaderModuleVk;
	class ContextVk;
	//

	class KS_API_DECL PipelineVk : public IPipeline
	{
		friend class GraphicsQueueAsyncTaskManager;
		friend class ContextVk;
	private:
		ContextVk* m_context;
		PipelineDescription	m_desc;
		VkGraphicsPipelineCreateInfo m_createInfo;
		VkPipeline m_pipelines[TopologyMode::TMCount];
		VkPipelineLayout m_pipelineLayout;
		std::vector<DescriptorSetLayout> m_vecDescriptorSetLayout;
		//
		VkRenderPass m_renderPass;
		//
		ShaderModuleVk* m_vertexModule;
		ShaderModuleVk* m_fragmentModule;
		//
		VkViewport m_viewport;
		VkRect2D m_scissor;
		VkCommandBuffer m_commandBuffer;
		float m_blendConstants[4];
		uint32_t m_stencilReference;
		float m_constantBias;
		float m_slopeScaleBias;
		//
		static VkPipelineCache GeneralPipelineCache;
	public:
		virtual void begin() override;
		virtual void end() override;
		virtual void setViewport(const Viewport& _viewport) override;
		virtual void setScissor(const Scissor& _scissor) override;

		virtual void bindVertexBuffer(uint32_t _index, IBuffer* _vertexBuffer, uint32_t _offset ) override;
		virtual void draw(TopologyMode _pmode, uint32_t _offset, uint32_t _vertexCount) override;
		virtual void drawIndexed(TopologyMode _pmode, IBuffer* _indexBuffer, uint32_t _indexOffset, uint32_t _indexCount) override;
		virtual void drawInstanced(TopologyMode _pmode, uint32_t _vertexOffset, uint32_t _vertexCount, uint32_t _instanceCount) override;
		virtual void drawInstanced(TopologyMode _pmode, IBuffer* _indexBuffer, uint32_t _indexOffset, uint32_t _indexCount, uint32_t _instanceCount) override;

		virtual void setPolygonOffset(float _constantBias, float _slopeScaleBias) override;
		virtual IArgument* createArgument(const ArgumentDescription& _desc) override;
		IArgument* createArgument(uint32_t _setId);
		virtual void setShaderCache(const void* _data, size_t _size, size_t _offset) override;
		virtual bool getVertexShaderCacheMember( const std::string _name, size_t& size_, size_t& offset_ );
		virtual bool getFragmentShaderCacheMember(const std::string _name, size_t& size_, size_t& offset_);
		virtual const PipelineDescription& getDescription() override;
		virtual void release() override;
	public:
		PipelineVk() :
			m_renderPass(VK_NULL_HANDLE),
			m_pipelineLayout(VK_NULL_HANDLE),
			m_vertexModule(nullptr),
			m_fragmentModule(nullptr),
			m_stencilReference(0),
			m_constantBias(0),
			m_slopeScaleBias(0)
		{
			m_blendConstants[0] = m_blendConstants[1] = m_blendConstants[2] = m_blendConstants[3] = 1.0f;
		}
		~PipelineVk();
		const std::vector<DescriptorSetLayout>& GetDescriptorSetLayouts() const {
			return m_vecDescriptorSetLayout;
		}
		VkPipeline requestPipelineObject( TopologyMode _topologyMode );
		VkPipelineLayout getPipelineLayout() const;
		bool getUniformMember(const char * _name, uint32_t _setId, uint32_t & index_, uint32_t & offset_);
		bool getSampler(const char * _name, uint32_t _setId, uint32_t& index_);
		void getUniformBindingPoints(size_t _setId, std::vector<UniformBindingPoint>& _bindingPoints);
		void getSamplerBindingPoints(uint32_t _setId, std::vector<SamplerBindingPoint>& _bindingPoints);
		//
		void setStencilReference( uint32_t _stencilReference );
		void setBlendFactor(const float* _blendFactors);
	public:
		//
		static VkPipelineLayout CreateRenderPipelineLayout(const std::vector<DescriptorSetLayout>& _descriptorSetLayouts, uint32_t _maxPushConstantSize );
	};
}