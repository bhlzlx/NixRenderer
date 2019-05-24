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
	struct DescriptorSetLayout {
		uint32_t						m_descriptorSetIndex;
		VkDescriptorSetLayout			m_descriptorSetLayout;
		std::vector<ShaderDescriptor>	m_descriptors;
		///////
		uint32_t getUniformLocation(const std::string& _name);
		uint32_t getSamplerLocation(const std::string& _name );
		uint32_t getSSBOLocation(const std::string& _name );
	};

	class MaterialVk : public IMaterial {
	private:
		MaterialDescription										m_description;
		ContextVk*												m_context;
		VkShaderModule											m_vertexShader;
		VkShaderModule											m_fragmentShader;
		uint32_t												m_descriptorSetLayoutCount;
		std::array<DescriptorSetLayout, MaxArgumentCount>		m_descriptorSetLayouts;
		//
		VkPipelineLayout			m_pipelineLayout;
	public:
		MaterialVk() :
			  m_vertexShader( VK_NULL_HANDLE )
			, m_fragmentShader( VK_NULL_HANDLE)
			, m_descriptorSetLayoutCount( 0 )
			, m_descriptorSetLayouts()
			, m_pipelineLayout( VK_NULL_HANDLE )
		{
		}

		virtual IArgument* createArgument( uint32_t _index ) override;

		virtual IRenderable* createRenderable() override;

		virtual IPipeline* createPipeline(IRenderPass* _renderPass) override;

		virtual void release() override;

		const DescriptorSetLayout& getDescriptorSetLayout(uint32_t _setIndex) const {
			return m_descriptorSetLayouts[_setIndex];
		}
		//
		static MaterialVk* CreateMaterial( ContextVk* _context, const MaterialDescription& _desc );
		static bool ValidationShaderDescriptor(const ShaderDescriptor& _descriptor, const uint32_t _setIndex, const spirv_cross::Compiler& _compiler, const spirv_cross::ShaderResources& _resources, ContextVk* _context);
	};

}