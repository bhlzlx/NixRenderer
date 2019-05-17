#include "MaterialVk.h"
#include "RenderableVk.h"
#include "ContextVk.h"

namespace Ks {

	ArgumentVk* MaterialVk::createArgument(uint32_t _index) {
		return nullptr;
	}

	RenderableVk* MaterialVk::createRenderable() {
		return nullptr;
	}
	//
	MaterialVk* MaterialVk::CreateMaterial( ContextVk* _context, MaterialDescription& _desc, const std::vector<uint32_t>& _vertSPV, const std::vector<uint32_t>& _fragSPV) {

		spirv_cross::Compiler vertCompiler( _vertSPV.data(), _vertSPV.size() );
		spirv_cross::ShaderResources vertexResource = vertCompiler.get_shader_resources();
		spirv_cross::Compiler fragCompiler(_fragSPV.data(), _fragSPV.size());
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
		for (uint32_t layoutIndex = 0; layoutIndex < _desc.argumentLayout.size(); ++layoutIndex) {
			std::vector<VkDescriptorSetLayoutBinding> bindings;
			for ( auto& uniform : _desc.argumentLayout[layoutIndex].uniformChunks) {
				VkDescriptorSetLayoutBinding binding;
				binding.binding = uniform.binding;
				binding.descriptorCount = 1;
				binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
				//binding.stageFlags = 
			}

			VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {}; {
				layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
				layoutCreateInfo.pNext = nullptr;
				layoutCreateInfo.flags = 0;
				layoutCreateInfo.bindingCount = _desc.argumentLayout[layoutIndex].samples.size() + _desc.argumentLayout[layoutIndex].uniformChunks.size();
				layoutCreateInfo.pBindings;
			}
		}
		//vkCreateDescriptorSetLayout
		VkShaderModuleCreateInfo info; {
			info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			info.pNext = nullptr;
			info.flags = 0;
			info.codeSize = spirvBytes.size() * sizeof(uint32_t);
			info.pCode = spirvBytes.data();
		}
		VkShaderModule module;
		if (VK_SUCCESS != vkCreateShaderModule(m_logicalDevice, &info, nullptr, &module)) {
			return nullptr;
		}
	}

}