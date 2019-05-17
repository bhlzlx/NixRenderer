#pragma once
#include <NixRenderer.h>
#include "vkinc.h"
#include <vector>
#include <string>

namespace Ks {

	struct SamplerBindingPoint
	{
		std::string		name;
		uint32_t		binding;
		uint32_t		arraySize;
	};

	struct UBOChunkMember
	{
		std::string		name;
		size_t			size;
		size_t			offset;
	};

	struct UniformBindingPoint
	{
		std::string		name;
		size_t			size;
		uint32_t		binding;
		std::vector<UBOChunkMember> members;
	};


	struct UniformMember
	{
		std::string		name;
		size_t			size;
		size_t			offset;
	};

	struct ConstantsMember {
		std::string		name;
		size_t			size;
		size_t			offset;
	};

	// shader module descriptor set
	struct SMSet 
	{
		uint32_t id;
		std::vector<SamplerBindingPoint> samplers;
		std::vector<UniformBindingPoint> uniforms;
	};

	class KS_API_DECL ShaderModuleVk
	{
		friend class ContextVk;
	private:
		VkShaderModule m_module;
		VkShaderStageFlagBits m_stage;
		std::vector< SMSet > m_sets;
		size_t m_constantsSize;
		std::vector< ConstantsMember > m_constants;
	public:
		ShaderModuleVk() :
			m_module(VK_NULL_HANDLE),
			m_stage(VK_SHADER_STAGE_VERTEX_BIT) {
		}
		//
		bool GetUniformMember(const char * _name, uint32_t& _set, uint32_t& _binding, uint32_t& _offset) {
			for (auto& set : m_sets)
			{
				for (auto& unif_block : set.uniforms)
				{
					for (auto& member : unif_block.members)
					{
						if (member.name == _name)
						{
							_set = set.id;
							_binding = unif_block.binding;
							return true;
						}
					}
				}
			}
			return false;
		}
		//
		bool GetSampler(const char * _name, uint32_t& _set, uint32_t& _binding) {
			for (auto& set : m_sets)
			{
				for (auto& sampler : set.samplers)
				{
					if (sampler.name == _name)
					{
						_set = set.id;
						_binding = sampler.binding;
						return true;
					}
				}
			}
			return false;
		}
		//
		const std::vector< SMSet >& GetDescriptorSetStructure() const {
			return m_sets;
		}

		uint32_t GetUniformBlockCount() const {
			uint32_t count = 0;
			for (auto& set : m_sets)
			{
				count += static_cast<uint32_t>(set.uniforms.size());
			}
			return count;
		}

		uint32_t GetSamplerCount() const {
			uint32_t count = 0;
			for (auto& set : m_sets)
			{
				count += static_cast<uint32_t>(set.samplers.size());
			}
			return count;
		}
		size_t GetConstantsSize() {
			return m_constantsSize;
		}
		bool GetConstantsMember(const std::string _name, size_t& size_, size_t& offset_) {
			for ( auto& member : m_constants )
			{
				if (member.name == _name) {
					size_ = member.size;
					offset_ = member.offset;
					return true;
				}
			}
			return false;
		}
		operator const VkShaderModule&() const {
			return m_module;
		}
		//
		static std::vector< DescriptorSetLayout > CreateDescriptorSetLayout(ShaderModuleVk* _vertexShader, ShaderModuleVk* _fragmentShader);
	};
}
