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
	struct ArgumentLayout {
		struct UniformMember {
			std::string	name;
			uint32_t	binding;
			uint32_t	offset;
		};
		uint32_t						m_descriptorSetIndex;
		VkDescriptorSetLayout			m_descriptorSetLayout;
		//std::vector<ShaderDescriptor>	m_descriptors;
		std::vector<ShaderDescriptor>	m_uniformBlockDescriptor;
		std::vector<ShaderDescriptor>	m_samplerImageDescriptor;
		std::vector<ShaderDescriptor>	m_storageBufferDescriptor;
		std::vector<ShaderDescriptor>	m_texelBufferDescriptor;


		std::vector<UniformMember>		m_uniformMembers;
		///////
		const ShaderDescriptor* getUniformBlock(const std::string& _name);
		uint32_t getUniformBlockMemberOffset( uint32_t _binding, const std::string& _name );
		const ShaderDescriptor* getSampler(const std::string& _name );
		const ShaderDescriptor* getSSBO(const std::string& _name );
	};

	class MaterialVk : public IMaterial {
	private:
		MaterialDescription										m_description;
		ContextVk*												m_context;
		VkShaderModule											m_vertexShader;
		VkShaderModule											m_fragmentShader;
		uint32_t												m_descriptorSetLayoutCount;
		std::array<ArgumentLayout, MaxArgumentCount>			m_argumentLayouts;
		//
		VkPipelineLayout										m_pipelineLayout;
	public:
		MaterialVk() :
			  m_vertexShader( VK_NULL_HANDLE )
			, m_fragmentShader( VK_NULL_HANDLE)
			, m_descriptorSetLayoutCount( 0 )
			, m_argumentLayouts()
			, m_pipelineLayout( VK_NULL_HANDLE )
		{
		}

		virtual IArgument* createArgument( uint32_t _index ) override;

		virtual IRenderable* createRenderable() override;

		virtual IPipeline* createPipeline(IRenderPass* _renderPass) override;

		virtual void release() override;

		const ArgumentLayout& getDescriptorSetLayout(uint32_t _setIndex) const {
			return m_argumentLayouts[_setIndex];
		}
		//
		static MaterialVk* CreateMaterial( ContextVk* _context, const MaterialDescription& _desc );
		static bool ValidationShaderDescriptor(const ShaderDescriptor& _descriptor, const uint32_t _setIndex, const spirv_cross::Compiler& _compiler, const spirv_cross::ShaderResources& _resources, ContextVk* _context, spirv_cross::Resource& res);
	};

}