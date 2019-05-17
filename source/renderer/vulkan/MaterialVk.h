#pragma once
#include "vkinc.h"
#include <KsRenderer.h>
#include <SPIRV-Cross/spirv_cross.hpp>

namespace Ks {
	//
	class ContextVk;
	class RenderableVk;
	class ArgumentVk;
	//
	class MaterialVk {
	private:
		ContextVk*					m_context;
		//
		//spirv_cross::ShaderResources m_vertexResource;
		//spirv_cross::ShaderResources m_fragmentResource;

		VkShaderModule				m_vertexShader;
		VkShaderModule				m_fragmentShader;
		//
		uint32_t					m_descriptorSetLayoutCount;
		VkDescriptorSetLayout		m_descriptorSetLayout[4];
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

		ArgumentVk* createArgument( uint32_t _index );		
		RenderableVk* createRenderable();
		//
		static MaterialVk* CreateMaterial( ContextVk* _context, MaterialDescription& _desc,  const std::vector<uint32_t>& _vertSPV, const std::vector<uint32_t>& _fragSPV );
	};

}