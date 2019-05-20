#pragma once
#include "vkinc.h"
#include <NixRenderer.h>
#include <array>
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
		std::array<VkDescriptorSetLayout, MaxArgumentCount>		m_descriptorSetLayouts;
		//
		VkPipelineLayout			m_pipelineLayout;
	public:
		MaterialVk() :
			  m_vertexShader( VK_NULL_HANDLE )
			, m_fragmentShader( VK_NULL_HANDLE)
			, m_descriptorSetLayoutCount( 0 )
			, m_descriptorSetLayouts( { VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE } )
			, m_pipelineLayout( VK_NULL_HANDLE )
		{
		}

		virtual IArgument* createArgument( uint32_t _index ) override;

		virtual IRenderable* createRenderable() override;

		virtual IPipeline* createPipeline(IRenderPass* _renderPass) override;

		virtual void release() override;

		VkDescriptorSetLayout getDescriptorSetLayout(uint32_t _setIndex) {
			return m_descriptorSetLayouts[_setIndex];
		}
		//
		static MaterialVk* CreateMaterial( ContextVk* _context, const MaterialDescription& _desc );
		static bool ValidationShaderDescriptor(const ShaderDescriptor& _descriptor, const uint32_t _setIndex, const spirv_cross::Compiler& _compiler, const spirv_cross::ShaderResources& _resources, ContextVk* _context);
	};

}