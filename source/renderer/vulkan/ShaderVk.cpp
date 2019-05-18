#include "ShaderVk.h"
#include "ContextVk.h"
#include "DescriptorSetVk.h"
#include "compiler/GLSLCompiler.h"
#include "compiler/SPIRVReflection.h"
#include <map>
#include <cassert>

namespace nix {

	std::vector< DescriptorSetLayout > ShaderModuleVk::CreateDescriptorSetLayout(ShaderModuleVk* _vertexShader, ShaderModuleVk* _fragmentShader)
	{
		std::map< uint32_t, VkDescriptorSetLayoutCreateInfo> setLayoutMap;
		std::map< uint32_t, std::vector< VkDescriptorSetLayoutBinding > > BindingMap;
		//z
		ShaderModuleVk* shaders[2] = { _vertexShader, _fragmentShader };
		for (uint32_t i = 0; i < 2; ++i)
		{
			if (!shaders[i])
				break;
			const auto& ss = shaders[i]->GetDescriptorSetStructure();
			for (auto& set : ss)
			{
				VkDescriptorSetLayoutCreateInfo* pCreateInfo = nullptr;
				auto iter = setLayoutMap.find(set.id);
				if (iter == setLayoutMap.end())
				{
					VkDescriptorSetLayoutCreateInfo setLayoutInfo; {
						setLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
						setLayoutInfo.pNext = nullptr;
						setLayoutInfo.flags = 0;//
						setLayoutInfo.bindingCount = 0;
						setLayoutInfo.pBindings = nullptr;
					}
					setLayoutMap[set.id] = setLayoutInfo;
					pCreateInfo = &setLayoutMap[set.id];
				}
				else
				{
					pCreateInfo = &iter->second;
				}
				pCreateInfo->bindingCount += static_cast<uint32_t>(set.samplers.size());
				pCreateInfo->bindingCount += static_cast<uint32_t>(set.uniforms.size());
			}
		}
		// construct bindings
		for (uint32_t i = 0; i < 2; ++i)
		{
			if (!shaders[i])
				break;
			const auto& ss = shaders[i]->GetDescriptorSetStructure();
			for (auto& set : ss)
			{
				auto& vecBinding = BindingMap[set.id];
				for (auto& sampler : set.samplers) {
					VkDescriptorSetLayoutBinding binding;
					{
						binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
						binding.descriptorCount = 1; // greater than 1 indicates that it's an array
						binding.stageFlags = (i == 0) ? VK_SHADER_STAGE_VERTEX_BIT : VK_SHADER_STAGE_FRAGMENT_BIT;
						binding.binding = sampler.binding;
						binding.pImmutableSamplers = nullptr;// sampler are bound at run time, not the creating time!
					}
					vecBinding.push_back(binding);
				}
				for (auto& uniform : set.uniforms) {
					VkDescriptorSetLayoutBinding binding;
					{
						binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
						binding.descriptorCount = 1; // greater than 1 indicates that it's an array
						binding.stageFlags = (i == 0) ? VK_SHADER_STAGE_VERTEX_BIT : VK_SHADER_STAGE_FRAGMENT_BIT;
						binding.binding = uniform.binding;
						binding.pImmutableSamplers = nullptr;// uniform binding, must not have samplers
					}
					vecBinding.push_back(binding);
				}
			}
		}
		//
		auto context = (ContextVk*)GetContextVulkan();
		//
		std::vector< DescriptorSetLayout > vecDescriptorLayout;
		for (auto& p : setLayoutMap)
		{
			p.second.pBindings = BindingMap[p.first].data();
			assert(p.second.bindingCount == BindingMap[p.first].size() && "count must be the same!");
			VkDescriptorSetLayout layout;
			VkResult rst = vkCreateDescriptorSetLayout(context->getDevice(), &p.second, nullptr, &layout);
			if (VK_SUCCESS != rst) {
				assert(false);
				return vecDescriptorLayout;
			}
			vecDescriptorLayout.push_back({ layout, p.first });
		}
		return vecDescriptorLayout;
	}

	ShaderModuleVk* ContextVk::createShaderModule(const char * _text, const char * _entryPoint, VkShaderStageFlagBits _stage) const {
		// step.1
		// compile glsl to spirv binary
		std::string compileInfo;
		std::vector< uint32_t > spirvBytes;
		bool rst = vez::CompileGLSL2SPIRV(_stage, std::string(_text), std::string(_entryPoint), spirvBytes, compileInfo);
		if (!rst) {
			printf("GLSL compiling error: %s\n", compileInfo.c_str());
			return nullptr;
		}
		// step.2
		// create VkShaderModule
		VkShaderModuleCreateInfo info; {
			info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			info.pNext = nullptr;
			info.flags = 0;
			info.codeSize = spirvBytes.size() * sizeof(uint32_t);
			info.pCode = spirvBytes.data();
		}
		VkShaderModule module;
		if (VK_SUCCESS != vkCreateShaderModule( m_logicalDevice, &info, nullptr, &module))
		{
			return nullptr;
		}
		// create ShaderModuleVk object & return
		ShaderModuleVk* shader = new ShaderModuleVk();
		shader->m_constantsSize = 0;
		shader->m_module = module;
		shader->m_stage = _stage;
		// step.3 
		// shader reflection
		std::vector<VezPipelineResource> shaderStageResource;
		vez::SPIRVReflectResources(spirvBytes, VK_SHADER_STAGE_VERTEX_BIT, shaderStageResource);
		//
		std::map< uint32_t, SMSet* > setMap;
		for (auto& res : shaderStageResource) {
			// only deal the `VEZ_PIPELINE_RESOURCE_TYPE_COMBINED_IMAGE_SAMPLER` & `VEZ_PIPELINE_RESOURCE_TYPE_UNIFORM_BUFFER`
			switch (res.resourceType)
			{
			case VEZ_PIPELINE_RESOURCE_TYPE_COMBINED_IMAGE_SAMPLER:
			{
				SMSet* pSet = nullptr;
				auto pos = setMap.find(res.set);
				if (pos == setMap.end())
				{
					shader->m_sets.push_back(SMSet());
					pSet = &shader->m_sets.back();
					pSet->id = res.set;
					setMap[res.set] = pSet;
				}
				else
				{
					pSet = pos->second;
				}
				SamplerBindingPoint point;
				point.binding = res.binding;
				point.name = res.name;
				point.arraySize = res.arraySize;
				pSet->samplers.push_back(point);
				break;
			}
			case VEZ_PIPELINE_RESOURCE_TYPE_UNIFORM_BUFFER:
			{
				SMSet* pSet = nullptr;
				auto pos = setMap.find(res.set);
				if (pos == setMap.end())
				{
					shader->m_sets.push_back(SMSet());
					pSet = &shader->m_sets.back();
					setMap[res.set] = pSet;
					pSet->id = res.set;
				}
				else
				{
					pSet = pos->second;
				}
				UniformBindingPoint point;
				point.name = res.name;
				point.binding = res.binding;
				point.size = res.size;
				auto member = res.pMembers;
				while (member)
				{
					UBOChunkMember item;
					item.name = member->name;
					item.size = member->size;
					item.offset = member->offset;
					point.members.push_back(item);
					member = member->pNext;
				}
				pSet->uniforms.push_back(point);
				break;
			}
			case VEZ_PIPELINE_RESOURCE_TYPE_PUSH_CONSTANT_BUFFER:
			{
				auto member = res.pMembers;
				while (member)
				{
					shader->m_constants.push_back({
						std::string(member->name),
						member->size,
						member->offset
					});
					member = member->pNext;
				}
				shader->m_constantsSize = res.size;
				break;
			}
			//
			case VEZ_PIPELINE_RESOURCE_TYPE_INPUT:
			case VEZ_PIPELINE_RESOURCE_TYPE_OUTPUT:
			case VEZ_PIPELINE_RESOURCE_TYPE_SAMPLER:
			case VEZ_PIPELINE_RESOURCE_TYPE_SAMPLED_IMAGE:
			case VEZ_PIPELINE_RESOURCE_TYPE_STORAGE_IMAGE:
			case VEZ_PIPELINE_RESOURCE_TYPE_UNIFORM_TEXEL_BUFFER:
			case VEZ_PIPELINE_RESOURCE_TYPE_STORAGE_TEXEL_BUFFER:
			case VEZ_PIPELINE_RESOURCE_TYPE_STORAGE_BUFFER:
			case VEZ_PIPELINE_RESOURCE_TYPE_INPUT_ATTACHMENT:
			default:
				break;
			}
		}
		//
		return shader;
	}
}