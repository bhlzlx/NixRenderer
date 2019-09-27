#include "MaterialVk.h"
#include "ContextVk.h"
#include "RenderableVk.h"
#include "DescriptorSetVk.h"
#include "TypemappingVk.h"
#include "ContextVk.h"
#include "DriverVk.h"

namespace Nix {

	VkShaderStageFlagBits NixShaderStageToVk(ShaderModuleType _stage) {
		switch (_stage)
		{
		case Nix::VertexShader:
			return VK_SHADER_STAGE_VERTEX_BIT;
		case Nix::FragmentShader:
			return VK_SHADER_STAGE_FRAGMENT_BIT;
		case Nix::ComputeShader:
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

	void MaterialVk::destroyArgument(IArgument* _argument)
	{
		return m_context->getDescriptorSetPool().free((ArgumentVk*)_argument);
	}

	void MaterialVk::release()
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

#define VULKAN_SHADER_PATH( SHADER_NAME ) "/shader/vulkan/" + std::string(SHADER_NAME)

	MaterialVk* MaterialVk::CreateMaterial(ContextVk* _context, const MaterialDescription& _desc)
	{
		MaterialDescription materialDesc = _desc;
		//
		VkShaderModule shaderModules[ShaderModuleType::ShaderTypeCount] = {};
		std::vector< VkPushConstantRange > constantRanges;
		uint32_t constantsStageFlags = 0;
		std::array<ArgumentLayoutExt, MaxArgumentCount> argumentLayouts;
		//
		VkDevice device = _context->getDevice();
		// 1. verify the vertex input layout
		// 2. verify the descriptor set description
		// 3. get the push constants information
		DriverVk* driver = (DriverVk*)_context->getDriver();
		auto compiler = driver->getShaderCompiler();
		for (const auto& shader : materialDesc.shaders) {
			if (shader.content) {
				shaderModules[shader.type] = CreateShaderModule(_context, shader.content, shader.type);
				// 这个switch其实是处理一些需要特殊验证
				switch (shader.type) {
				case ShaderModuleType::VertexShader: {
					// validate the vertex layout
					const StageIOAttribute* vertexInputs = nullptr;
					uint16_t vertexAttrCount = compiler->getStageInput(&vertexInputs);
					if (vertexAttrCount != _desc.vertexLayout.attributeCount) {
						assert(false && "layout does not match!");
						return nullptr;
					}
					for (uint32_t attrIndex = 0; attrIndex < materialDesc.vertexLayout.attributeCount; ++attrIndex) {
						//
						auto& descAttr = materialDesc.vertexLayout.attributes[attrIndex];
						auto& compilerAttr = vertexInputs[attrIndex];
						assert(compilerAttr.location == attrIndex);
						if (strcmp(compilerAttr.name, descAttr.name) != 0) {
							assert(false && "name does not match!");
							return nullptr;
						}
						if ( compilerAttr.type != descAttr.type ) {
							assert(false && "vertex type does not match!");
							return nullptr;
						}
					}
				}
				case ShaderModuleType::FragmentShader: {
				}
				case ShaderModuleType::ComputeShader: {
				}
				case ShaderModuleType::TessellationShader: {
				}
				case ShaderModuleType::GeometryShader: {
				}
				default: {
				}
				}
				// 处理 descriptor
				const UniformBlock* uniforms;
				uint16_t numUnif = compiler->getUniformBlocks(&uniforms);
				for (uint16_t i = 0; i < numUnif; ++i) {
					const UniformBlock& unif = uniforms[i];
					auto& argument = argumentLayouts[unif.set];
					argument.m_vecUniformBlock.push_back(unif);
				}
				const CombinedImageSampler* samplers;
				uint16_t numSampler = compiler->getSamplers(&samplers);
				for (uint16_t i = 0; i < numSampler; ++i) {
					const CombinedImageSampler& sampler = samplers[i];
					auto& argument = argumentLayouts[sampler.set];
					argument.m_vecSampler.push_back(sampler);
				}
				const ShaderStorageBufferObject* ssbos;
				uint16_t numSsbo = compiler->getShaderStorageBuffers(&ssbos);
				for (uint16_t i = 0; i < numSsbo; ++i) {
					const ShaderStorageBufferObject& ssbo = ssbos[i];
					auto& argument = argumentLayouts[ssbo.set];
					argument.m_vecSSBO.push_back(ssbo);
				}
				const TexelBufferObject* tbos;
				uint16_t numTbo = compiler->getTexelBuffer(&tbos);
				for (uint16_t i = 0; i < numTbo; ++i) {
					const TexelBufferObject& tbo = tbos[i];
					auto& argument = argumentLayouts[tbo.set];
					argument.m_vecTBO.push_back(tbo);
				}
				const SubpassInput* subpassInputs;
				uint16_t numSubpass = compiler->getInputAttachment(&subpassInputs);
				for (uint16_t i = 0; i < numSubpass; ++i) {
					const SubpassInput& attachment = subpassInputs[i];
					auto& argument = argumentLayouts[attachment.set];
					argument.m_vecSubpassInput.push_back(attachment);
				}
			}
			PushConstants pc;
			compiler->getPushConstants(&pc.offset, &pc.size);
			VkPushConstantRange vkpc;
			vkpc.offset = pc.offset;
			vkpc.size = pc.size;
			VkShaderStageFlagBits shaderStages[] = {
				VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT,
				VkShaderStageFlagBits::VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,
				VkShaderStageFlagBits::VK_SHADER_STAGE_GEOMETRY_BIT,
				VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT,
				VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT,
			};
			vkpc.stageFlags = shaderStages[shader.type];
			constantsStageFlags |= vkpc.stageFlags;
			constantRanges.push_back(vkpc);
		}
		//
		VkDescriptorSetLayout setLayouts[MaxArgumentCount];
		for (uint32_t setIndex = 0; setIndex < materialDesc.argumentCount; ++setIndex) {
			std::vector<VkDescriptorSetLayoutBinding> bindings;
			auto& argument = materialDesc.argumentLayouts[setIndex];
            for( uint32_t dscIndex = 0; dscIndex < argument.descriptorCount; ++dscIndex){
                auto& descriptor = argument.descriptors[dscIndex];
				switch (descriptor.type) {
				case SDT_Sampler: {
					VkDescriptorSetLayoutBinding binding;
					binding.binding = descriptor.binding;
					binding.descriptorCount = 1;
					binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
					binding.stageFlags = NixShaderStageToVk(descriptor.shaderStage);
					binding.pImmutableSamplers = nullptr;
					bindings.push_back(binding);
					break;
				}
				case SDT_UniformBuffer: {
					VkDescriptorSetLayoutBinding binding;
					binding.binding = descriptor.binding;
					binding.descriptorCount = 1;
					binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
					binding.stageFlags = NixShaderStageToVk(descriptor.shaderStage);
					binding.pImmutableSamplers = nullptr;
					bindings.push_back(binding);
					break;
				}
				case SDT_StorageBuffer: {
					VkDescriptorSetLayoutBinding binding;
					binding.binding = descriptor.binding;
					binding.descriptorCount = 1;
					binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
					binding.stageFlags = NixShaderStageToVk(descriptor.shaderStage);
					binding.pImmutableSamplers = nullptr;
					bindings.push_back(binding);
					break;
				}
				case SDT_UniformTexelBuffer: {
					VkDescriptorSetLayoutBinding binding;
					binding.binding = descriptor.binding;
					binding.descriptorCount = 1;
					binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
					binding.stageFlags = NixShaderStageToVk(descriptor.shaderStage);
					binding.pImmutableSamplers = nullptr;
					bindings.push_back(binding);
					break;
				}
				case SDT_StorageTexelBuffer: {
					VkDescriptorSetLayoutBinding binding;
					binding.binding = descriptor.binding;
					binding.descriptorCount = 1;
					binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
					binding.stageFlags = NixShaderStageToVk(descriptor.shaderStage);
					binding.pImmutableSamplers = nullptr;
					bindings.push_back(binding);
					break;
				}
				case SDT_InputAttachment: {
					VkDescriptorSetLayoutBinding binding;
					binding.binding = descriptor.binding;
					binding.descriptorCount = 1;
					binding.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
					binding.stageFlags = NixShaderStageToVk(descriptor.shaderStage);
					binding.pImmutableSamplers = nullptr;
					bindings.push_back(binding);
				}
				}
            }
			VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {}; {
				layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
				layoutCreateInfo.pNext = nullptr;
				layoutCreateInfo.flags = 0;
				layoutCreateInfo.bindingCount = (uint32_t)bindings.size();
				layoutCreateInfo.pBindings = bindings.data();
			}
			vkCreateDescriptorSetLayout(device, &layoutCreateInfo, nullptr, &setLayouts[setIndex]);
			argumentLayouts[setIndex].m_setLayout = setLayouts[setIndex];
			argumentLayouts[setIndex].m_setID = setIndex;
			//
			argumentLayouts[setIndex].completeSetup(_context);
		}
		// create pipeline layout
		VkPipelineLayoutCreateInfo info; {
			info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			info.pNext = nullptr;
			info.flags = 0;
			info.pSetLayouts = setLayouts;
			info.setLayoutCount = static_cast<uint32_t>(materialDesc.argumentCount);
			info.pushConstantRangeCount = (uint32_t)constantRanges.size();
			info.pPushConstantRanges = constantRanges.size() ? &constantRanges[0] : nullptr;
		}
		VkPipelineLayout pipelineLayout;
		VkResult rst = vkCreatePipelineLayout(device, &info, nullptr, &pipelineLayout);
		assert(rst == VK_SUCCESS);
		//
		MaterialVk* material = new MaterialVk();
		material->m_description = materialDesc;
		material->m_context = _context;
		memcpy(material->m_shaderModules, shaderModules, sizeof(shaderModules));
		material->m_descriptorSetLayoutCount = (uint32_t)materialDesc.argumentCount;
		material->m_pipelineLayout = pipelineLayout;
		material->m_argumentLayouts = argumentLayouts;
		material->m_topology = NixTopologyToVk( materialDesc.topologyMode );
		material->m_pologonMode = NixPolygonModeToVk(materialDesc.pologonMode);
		material->m_constantsStageFlags = constantsStageFlags;
		//
		return material;
	}

	//const ShaderDescriptor* ArgumentLayout::getUniformBlock(const std::string& _name)
	//{
	//	uint32_t i = 0;
	//	for (; i < m_uniformBlockDescriptor.size(); ++i) {
	//		if (m_uniformBlockDescriptor.at(i).type == SDT_UniformBuffer) {
	//			if (m_uniformBlockDescriptor.at(i).name == _name) {
	//				return &m_uniformBlockDescriptor.at(i);
	//			}
	//		}
	//	}
	//	return nullptr;
	//}

	//uint32_t ArgumentLayout::getUniformBlockMemberOffset(uint32_t _binding, const std::string& _name)
	//{
	//	for (auto& member : m_uniformMembers) {
	//		if (member.binding == _binding && member.name == _name ) {
	//			return member.offset;
	//		}
	//	}
	//	return -1;
	//}

	//const ShaderDescriptor* ArgumentLayout::getSampler(const std::string& _name)
	//{
	//	uint32_t i = 0;
	//	for (; i < m_samplerImageDescriptor.size(); ++i) {
	//		if (m_samplerImageDescriptor[i].type == SDT_Sampler) {
	//			if (m_samplerImageDescriptor[i].name == _name) {
	//				return &m_samplerImageDescriptor[i];
	//			}
	//		}
	//	}
	//	return nullptr;
	//}

	//const ShaderDescriptor* ArgumentLayout::getSSBO(const std::string& _name)
	//{
	//	uint32_t i = 0;
	//	for (; i < m_storageBufferDescriptor.size(); ++i) {
	//		if (m_storageBufferDescriptor[i].type == SDT_SSBO) {
	//			if (m_storageBufferDescriptor[i].name == _name) {
	//				return &m_storageBufferDescriptor[i];
	//			}
	//		}
	//	}
	//	return nullptr;
	//}

	//void ArgumentLayout::updateDynamicalBindings()
	//{
	//	// only uniform buffer object need dynamical bindings
	//	for (auto& descriptor : m_uniformBlockDescriptor) {
	//		m_dynamicalBindings.push_back(descriptor.binding);
	//	}
	//	//
	//	std::sort(m_dynamicalBindings.begin(), m_dynamicalBindings.end());
	//}

	VkShaderModule MaterialVk::CreateShaderModule( ContextVk* _context, const char * _shader, ShaderModuleType _type ) {
		auto length = strlen(_shader);
		//const char* compileInfo;
		const uint32_t* spvBytes;
		size_t spvLength;
		// 
		if (length < 96 && length > 4) { // if @_shader is a file path
			VkDevice device = _context->getDevice();
			// load SPV from disk!
			auto arch = _context->getDriver()->getArchieve();
			Nix::IFile * file = arch->open(VULKAN_SHADER_PATH(_shader));
			if (!file) { 
                assert(false); 
                return VK_NULL_HANDLE; 
            }
			Nix::IFile* mem = CreateMemoryBuffer(file->size() + 1);
			mem->seek(SeekEnd, -1);
			char endle = 0;
			mem->write(1, &endle);
			mem->seek(SeekSet, 0);
			file->read(file->size(), mem);
			file->release();

			DriverVk* driver = (DriverVk*)_context->getDriver();
			auto compiler = driver->getShaderCompiler();

			if (*(const char*)mem->constData() == '#') {
				bool rst = compiler->compile(_type, (const char*)mem->constData());
				if (!rst) {
					mem->release();
					return VK_NULL_HANDLE;
				}
				rst = compiler->getCompiledSpv(&spvBytes, &spvLength);
				rst = compiler->parseSpvLayout(_type, spvBytes, spvLength);
				VkShaderModule module = NixCreateShaderModule(device, spvBytes, spvLength * sizeof(uint32_t));
				return module;
			} else {
				auto module = NixCreateShaderModule(device, mem->constData(), mem->size() - 1 );
				bool rst = compiler->parseSpvLayout(_type, (uint32_t*)mem->constData(), mem->size() / sizeof(uint32_t));
				mem->release();
				return module;
			}
		}
		else
		{
			if (*_shader == '#') { // @_shader is shader text content
				DriverVk* driver = (DriverVk*)_context->getDriver();
				auto compiler = driver->getShaderCompiler();
				bool rst = compiler->compile(_type, _shader);
				if (!rst) {
					return VK_NULL_HANDLE;
				}
				rst = compiler->getCompiledSpv(&spvBytes, &spvLength);
				rst = compiler->parseSpvLayout(_type, spvBytes, spvLength);
				VkShaderModule module = NixCreateShaderModule(_context->getDevice(), spvBytes, spvLength * sizeof(uint32_t));
				return module;
			}
		}
		return VK_NULL_HANDLE;
	}

	const UniformBlock * ArgumentLayoutExt::getUniform(const std::string & _name) const {
		for (const auto& uniform : m_vecUniformBlock) {
			if (uniform.name == _name) {
				return &uniform;
			}
		}
		return nullptr;
	}

	const UniformBlock::Member * ArgumentLayoutExt::getUniformMember(uint32_t _binding, const std::string & _name) const {
		auto it = m_uniformLayouts.find(_binding);
		if ( it == m_uniformLayouts.end()) {
			return nullptr;
		}
		for (auto& member : it->second) {
			if (member.name == _name) {
				return &member;
			}
		}
		return nullptr;
	}

	const CombinedImageSampler * ArgumentLayoutExt::getSampler(const std::string & _name) const {
		for (const auto& sampler : m_vecSampler) {
			if (sampler.name == _name) {
				return &sampler;
			}
		}
		return nullptr;
	}

	const ShaderStorageBufferObject * ArgumentLayoutExt::getSSBO(const std::string & _name) const {
		for (const auto& ssbo : m_vecSSBO) {
			if (ssbo.name == _name) {
				return &ssbo;
			}
		}
		return nullptr;
	}

	const TexelBufferObject * ArgumentLayoutExt::getTBO(const std::string & _name) const {
		for (const auto& tbo : m_vecTBO) {
			if (tbo.name == _name) {
				return &tbo;
			}
		}
		return nullptr;
	}

	const SubpassInput * ArgumentLayoutExt::getSubpassInput(const std::string & _name) const {
		for (const auto& attachment : m_vecSubpassInput) {
			if (attachment.name == _name) {
				return &attachment;
			}
		}
		return nullptr;
	}

	void ArgumentLayoutExt::completeSetup(ContextVk* _context) {
		// calculate the uniform usage, every block's local offset, totabl block size
		DriverVk* driver = (DriverVk*)_context->getDriver();
		uint16_t uniformAlignment = driver->getPhysicalDeviceProperties().limits.minUniformBufferOffsetAlignment;
		size_t unifIndex = 0;
		m_UBOLocalOffsets.clear();
		m_UBOLocalSize = 0;
		for (; unifIndex < m_vecUniformBlock.size(); ++unifIndex) {
			m_UBOLocalOffsets.push_back(m_UBOLocalSize);
			m_UBOLocalSize += m_vecUniformBlock.size();
			m_UBOLocalSize = (m_UBOLocalSize + (uniformAlignment - 1))&~(uniformAlignment - 1);
		}
	}
}