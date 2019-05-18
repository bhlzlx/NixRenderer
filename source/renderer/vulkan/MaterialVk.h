#pragma once
#include "vkinc.h"
#include <NixRenderer.h>
#include <SPIRV-Cross/spirv_cross.hpp>

namespace nix {
	//
	class ContextVk;
	class RenderableVk;
	class ArgumentVk;
	//
	class MaterialVk : public IMaterial {
	private:
		MaterialDescription			m_description;
		//
		ContextVk*					m_context;
		//
		VkShaderModule				m_vertexShader;
		VkShaderModule				m_fragmentShader;
		//
		uint32_t					m_descriptorSetLayoutCount;
		VkDescriptorSetLayout		m_descriptorSetLayout[MaxArgumentCount];
		//
		VkPipelineLayout			m_pipelineLayout;
	public:
		MaterialVk() :
			  m_vertexShader( VK_NULL_HANDLE )
			, m_fragmentShader( VK_NULL_HANDLE)
			, m_descriptorSetLayoutCount( 0 )
			, m_pipelineLayout( VK_NULL_HANDLE )
		{
		}

		virtual IArgument* createArgument( uint32_t _index ) override;

		virtual IRenderable* createRenderable() override;

		virtual IPipeline* createPipeline(IRenderPass* _renderPass) override;

		virtual void release() override;
		//
		static MaterialVk* CreateMaterial( ContextVk* _context, const MaterialDescription& _desc );
	};

}