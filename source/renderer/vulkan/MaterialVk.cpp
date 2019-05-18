#include "MaterialVk.h"
#include "RenderableVk.h"
#include "ContextVk.h"

namespace nix {

	VkShaderStageFlagBits NixShaderStageToVk(ShaderModuleType _stage) {
		switch (_stage)
		{
		case nix::VertexShader:
			return VK_SHADER_STAGE_VERTEX_BIT;
		case nix::FragmentShader:
			return VK_SHADER_STAGE_FRAGMENT_BIT;
		case nix::ComputeShader:
			return VK_SHADER_STAGE_COMPUTE_BIT;
		default:
			break;
		}
		return VK_SHADER_STAGE_ALL;
	}

	VkShaderModule NixCreateShaderModule(VkDevice _device, const void * _pCode, size_t _codeSize) {
		VkShaderModuleCreateInfo info; {
			info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			info.pNext = nullptr;
			info.flags = 0;
			info.codeSize = _codeSize;
			info.pCode = (const uint32_t*)_pCode;
		}
		VkShaderModule module;
		if (VK_SUCCESS != vkCreateShaderModule( _device, &info, nullptr, &module)) {
			return VK_NULL_HANDLE;
		}
		return module;
	}

	IMaterial* ContextVk::createMaterial(const MaterialDescription& _desc) {
		return MaterialVk::CreateMaterial(this, _desc);
	}

	IArgument* MaterialVk::createArgument( uint32_t _index )
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	IPipeline* MaterialVk::createPipeline(IRenderPass* _renderPass)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void MaterialVk::release()
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	IRenderable* MaterialVk::createRenderable()
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

#define VULKAN_SHADER_PATH( SHADER_NAME ) "/shader/vulkan/" + std::string(SHADER_NAME)

	MaterialVk* MaterialVk::CreateMaterial(ContextVk* _context, const MaterialDescription& _desc)
	{
		VkDevice device = _context->getDevice();
		// load SPV from disk!
		auto arch = _context->getDriver()->getArchieve();
		nix::IFile * vertexSPV = arch->open(VULKAN_SHADER_PATH(_desc.vertexShader));
		if (!vertexSPV) { assert(false); return nullptr; }
		nix::IFile * fragmentSPV = arch->open(VULKAN_SHADER_PATH(_desc.vertexShader));
		if (!vertexSPV) { assert(false); return nullptr; }
		// create shader module
		VkShaderModule vertSM = NixCreateShaderModule(device, vertexSPV->constData(), vertexSPV->size());
		if (VK_NULL_HANDLE == vertSM) {
			assert(false); return nullptr;
		}
		VkShaderModule fragSM = NixCreateShaderModule(device, fragmentSPV->constData(), fragmentSPV->size());
		if (VK_NULL_HANDLE == fragSM) {
			assert(false); return nullptr;
		}
		// reflect the resource information
		spirv_cross::Compiler vertCompiler((const uint32_t*)fragmentSPV->constData(), fragmentSPV->size() / sizeof(uint32_t));
		spirv_cross::ShaderResources vertexResource = vertCompiler.get_shader_resources();
		spirv_cross::Compiler fragCompiler( (const uint32_t*)fragmentSPV->constData(), fragmentSPV->size() / sizeof(uint32_t) );
		spirv_cross::ShaderResources fragmentResource = fragCompiler.get_shader_resources();
		// 1. verify the vertex input layout
		// 2. verify the descriptor set description
		// 3. get the push constants information

		//\ 1 - vertex layout validation
		if (_desc.vertexLayout.vertexAttributeCount != vertexResource.stage_inputs.size()) {
			return nullptr;
		}
		for (auto& vertexInput : vertexResource.stage_inputs) {
			auto location = vertCompiler.get_decoration(vertexInput.id, spv::Decoration::DecorationLocation);
			if (_desc.vertexLayout.vertexAttributes[location].name != vertexInput.name) {
				assert("name does not match!" && nullptr);
				return nullptr;
			}
		}
		//\ 2 - descriptor set validation
		struct ShaderRes {
			spirv_cross::ShaderResources* resource;
			spirv_cross::Compiler* compiler;
		};

		ShaderRes shaderRes[2] = {
			{ &vertexResource, &vertCompiler },
			{ &fragmentResource, &fragCompiler }
		};
		//
		for (uint32_t setIndex = 0; setIndex < _desc.argumentLayout.size(); ++setIndex) {
			auto& argmentLayout = _desc.argumentLayout[setIndex];
			bool found = false;
			for (auto& uniform : argmentLayout.uniformChunks) {
				for (auto& shaderR : shaderRes) {
					for (auto& res : shaderR.resource->uniform_buffers) {
						if (res.name == uniform.name) {
							auto set = shaderR.compiler->get_decoration(res.id, spv::Decoration::DecorationDescriptorSet);
							auto binding = shaderR.compiler->get_decoration(res.id, spv::Decoration::DecorationBinding);
							if (set == setIndex && binding == uniform.binding) {
								auto uniformType = shaderR.compiler->get_type_from_variable(res.id);
								auto uniformChunkSize = shaderR.compiler->get_declared_struct_size(uniformType);
								if (uniformChunkSize == uniform.dataSize) {
									found = true;
								}
								else {
									_context->getDriver()->getLogger()->error("uniform chunk size mismatch!");
									return nullptr;
								}
							}
							else
							{
								found = false;
								_context->getDriver()->getLogger()->error(" UNIFORM : descriptor not found!");
								return nullptr;
							}
							break;
						}
					}
				}
				if (!found) {
					_context->getDriver()->getLogger()->error("UNIFORM :descriptor not found!");
					return nullptr;
				}
			}
			for (auto& sampler : argmentLayout.samples) {
				for (auto& resource : shaderRes) {
					for (auto& res : shaderRes->resource->sampled_images) {
						if (res.name == sampler.name) {
							auto set = shaderRes->compiler->get_decoration(res.id, spv::Decoration::DecorationDescriptorSet);
							auto binding = shaderRes->compiler->get_decoration(res.id, spv::Decoration::DecorationBinding);
							if (set == setIndex && binding == sampler.binding) {
								found = true;
							}
							else
							{
								found = false;
								_context->getDriver()->getLogger()->error(" SAMPLER : descriptor not found!");
							}
							break;
						}
					}
				}
				if (!found) {
					_context->getDriver()->getLogger()->error(" SAMPLER : descriptor not found!");
					return nullptr;
				}
			}
			for (auto& ssbo : argmentLayout.SSBOs) {
				for (auto& resource : shaderRes) {
					for (auto& res : shaderRes->resource->storage_buffers ) {
						if (res.name == ssbo.name) {
							auto set = shaderRes->compiler->get_decoration(res.id, spv::Decoration::DecorationDescriptorSet);
							auto binding = shaderRes->compiler->get_decoration(res.id, spv::Decoration::DecorationBinding);
							if (set == setIndex && binding == ssbo.binding) {
								found = true;
							}
							else
							{
								found = false;
								_context->getDriver()->getLogger()->error(" SSBO : descriptor not found!");
							}
							break;
						}
					}
				}
				if (!found) {
					_context->getDriver()->getLogger()->error(" SSBO : descriptor not found!");
					return nullptr;
				}
			}
		}
		//\ 3 - push constants information
		std::vector< VkPushConstantRange > constantRanges;

		if (vertexResource.push_constant_buffers.size()) {
			auto& vc = vertexResource.push_constant_buffers[0];
			auto ranges = vertCompiler.get_active_buffer_ranges(vc.id);
			VkPushConstantRange range = {};
			range.offset = ranges[0].offset;
			range.size = ranges[0].range;
			range.stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT;
			constantRanges.push_back(range);
		}

		if (fragmentResource.push_constant_buffers.size()) {
			auto& fc = fragmentResource.push_constant_buffers[0];
			auto ranges = vertCompiler.get_active_buffer_ranges(fc.id);
			VkPushConstantRange range = {};
			range.offset = ranges[0].offset;
			range.size = ranges[0].range;
			range.stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT;
			constantRanges.push_back(range);
		}
		//
		VkDescriptorSetLayout layouts[MaxArgumentCount];
		for (uint32_t layoutIndex = 0; layoutIndex < _desc.argumentLayout.size(); ++layoutIndex) {
			std::vector<VkDescriptorSetLayoutBinding> bindings;
			for (auto& uniform : _desc.argumentLayout[layoutIndex].uniformChunks) {
				VkDescriptorSetLayoutBinding binding;
				binding.binding = uniform.binding;
				binding.descriptorCount = 1;
				binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
				binding.stageFlags = NixShaderStageToVk(uniform.shaderStage);
				binding.pImmutableSamplers = nullptr;
				bindings.push_back(binding);
			}
			for (auto& sampler : _desc.argumentLayout[layoutIndex].samples) {
				VkDescriptorSetLayoutBinding binding;
				binding.binding = sampler.binding;
				binding.descriptorCount = 1;
				binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				binding.stageFlags = NixShaderStageToVk(sampler.shaderStage);
				binding.pImmutableSamplers = nullptr;
				bindings.push_back(binding);
			}
			for ( auto& ssbo : _desc.argumentLayout[layoutIndex].SSBOs ) {
				VkDescriptorSetLayoutBinding binding;
				binding.binding = ssbo.binding;
				binding.descriptorCount = 1;
				binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				binding.stageFlags = NixShaderStageToVk(ssbo.shaderStage);
				binding.pImmutableSamplers = nullptr;
				bindings.push_back(binding);
			}

			VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {}; {
				layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
				layoutCreateInfo.pNext = nullptr;
				layoutCreateInfo.flags = 0;
				layoutCreateInfo.bindingCount = bindings.size();
				layoutCreateInfo.pBindings = bindings.data();
			}
			vkCreateDescriptorSetLayout(device, &layoutCreateInfo, nullptr, &layouts[layoutIndex]);
		}
		// create pipeline layout
		VkPipelineLayoutCreateInfo info; {
			info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			info.pNext = nullptr;
			info.flags = 0;
			info.pSetLayouts = layouts;
			info.setLayoutCount = static_cast<uint32_t>(_desc.argumentLayout.size());
			info.pushConstantRangeCount = constantRanges.size();
			info.pPushConstantRanges = constantRanges.size() ? &constantRanges[0] : nullptr;
		}
		VkPipelineLayout pipelineLayout;
		VkResult rst = vkCreatePipelineLayout(device, &info, nullptr, &pipelineLayout);
		assert(rst == VK_SUCCESS);
		//
		MaterialVk* material = new MaterialVk();
		material->m_description = _desc;
		material->m_context = _context;
		material->m_vertexShader = vertSM;
		material->m_fragmentShader = fragSM;
		material->m_descriptorSetLayoutCount = _desc.argumentLayout.size();
		material->m_pipelineLayout = pipelineLayout;
		memcpy(material->m_descriptorSetLayout, layouts, sizeof(layouts));
		//
		return material;
	}

}