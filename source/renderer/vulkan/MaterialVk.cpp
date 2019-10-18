#include "MaterialVk.h"
#include "ContextVk.h"
#include "RenderableVk.h"
#include "DescriptorSetVk.h"
#include "TypemappingVk.h"
#include "ContextVk.h"
#include "DriverVk.h"

namespace Nix {

	void copyDescriptorSetLayoutToMaterial(std::array<ArgumentLayoutExt, MaxArgumentCount>& _layouts, MaterialDescription& _description) {
		uint32_t descriptorSetCounter = 0;
		for (uint32_t setId = 0; setId < MaxArgumentCount; ++setId) {
			auto& argument = _layouts[setId];
			const auto& descriptors = argument.getDescriptors();
			if (descriptors.size()) {
				_description.argumentLayouts[descriptorSetCounter].index = setId;
				_description.argumentLayouts[descriptorSetCounter].descriptorCount = descriptors.size();
				memcpy(_description.argumentLayouts[descriptorSetCounter].descriptors, descriptors.data(), descriptors.size() * sizeof(ShaderDescriptor));
				++descriptorSetCounter;
			}
		}
		_description.argumentCount = descriptorSetCounter;
	}

	VkShaderStageFlagBits NixShaderStageToVk(ShaderModuleType _stage) {
		switch (_stage)
		{
		case Nix::VertexShader:
			return VK_SHADER_STAGE_VERTEX_BIT;
		case Nix::FragmentShader:
			return VK_SHADER_STAGE_FRAGMENT_BIT;
		case Nix::ComputeShader:
			return VK_SHADER_STAGE_COMPUTE_BIT;
		case Nix::TessellationShader:
			return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
		case Nix::GeometryShader:
			return VK_SHADER_STAGE_GEOMETRY_BIT;
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
		if (VK_SUCCESS != vkCreateShaderModule(_device, &info, nullptr, &module)) {
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

		auto uniformAligment = driver->getPhysicalDeviceProperties().limits.minUniformBufferOffsetAlignment;
		auto storageAlignMent = driver->getPhysicalDeviceProperties().limits.minStorageBufferOffsetAlignment;

		for (const auto& shader : materialDesc.shaders) {
			if (shader.name[0]) {
				shaderModules[shader.type] = CreateShaderModule(_context, shader.name, shader.type);
				// 这个switch其实是处理一些需要特殊验证
				switch (shader.type) {
				case ShaderModuleType::VertexShader: {
					// validate the vertex layout
					const StageIOAttribute* vertexInputs = nullptr;
					uint16_t vertexAttrCount = compiler->getStageInput(&vertexInputs);
					if (vertexAttrCount != materialDesc.vertexLayout.attributeCount) {
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
						if (compilerAttr.type != descAttr.type) {
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
				const UniformBuffer* uniforms;
				uint16_t numUnif = compiler->getUniformBuffers(&uniforms);
				for (uint16_t i = 0; i < numUnif; ++i) {
					const UniformBuffer& unif = uniforms[i];
					auto& argument = argumentLayouts[unif.set];
					argument.m_vecUniformBuffer.push_back(unif);
					Nix::ShaderDescriptor descriptor = {}; {
						descriptor.binding = unif.binding;
						descriptor.dataSize = unif.size;
						descriptor.shaderStage = shader.type;
						strcpy(descriptor.name, unif.name);
						descriptor.type = SDT_UniformBuffer;
					}
					argument.m_vecShaderDescriptor.push_back(descriptor);
				}
				const StorageBuffer* ssbos;
				uint16_t numSsbo = compiler->getShaderStorageBuffers(&ssbos);
				for (uint16_t i = 0; i < numSsbo; ++i) {
					const StorageBuffer& ssbo = ssbos[i];
					auto& argument = argumentLayouts[ssbo.set];
					argument.m_vecStorageBuffer.push_back(ssbo);
					Nix::ShaderDescriptor descriptor = {}; {
						descriptor.binding = ssbo.binding;
						descriptor.dataSize = ssbo.size;
						descriptor.shaderStage = shader.type;
						strcpy(descriptor.name, ssbo.name);
						descriptor.type = SDT_StorageBuffer;
					}
					argument.m_vecShaderDescriptor.push_back(descriptor);
				}
				//
				const SeparateSampler* ssampler = nullptr;
				uint16_t numSampler = compiler->getSamplers(&ssampler);
				for (uint16_t i = 0; i < numSampler; ++i) {
					const SeparateSampler& sampler = ssampler[i];
					auto& argument = argumentLayouts[sampler.set];
					argument.m_vecSampler.push_back(sampler);
					Nix::ShaderDescriptor descriptor = {}; {
						descriptor.binding = sampler.binding;
						descriptor.dataSize = 0;
						descriptor.shaderStage = shader.type;
						strcpy(descriptor.name, sampler.name);
						descriptor.type = SDT_Sampler;
					}
					argument.m_vecShaderDescriptor.push_back(descriptor);
				}
				const CombinedImageSampler* csamplers;
				numSampler = compiler->getCombinedImageSampler(&csamplers);
				for (uint16_t i = 0; i < numSampler; ++i) {
					const CombinedImageSampler& sampler = csamplers[i];
					auto& argument = argumentLayouts[sampler.set];
					argument.m_vecCombinedImageSampler.push_back(sampler);
					Nix::ShaderDescriptor descriptor = {}; {
						descriptor.binding = sampler.binding;
						descriptor.dataSize = 0;
						descriptor.shaderStage = shader.type;
						strcpy(descriptor.name, sampler.name);
						descriptor.type = SDT_SamplerImage;
					}
					argument.m_vecShaderDescriptor.push_back(descriptor);
				}
				const SeparateImage* simage;
				uint16_t numImage = compiler->getSampledImages(&simage);
				for (uint16_t i = 0; i < numImage; ++i) {
					const SeparateImage& sampler = simage[i];
					auto& argument = argumentLayouts[sampler.set];
					argument.m_vecSampledImage.push_back(sampler);
					Nix::ShaderDescriptor descriptor = {}; {
						descriptor.binding = sampler.binding;
						descriptor.dataSize = 0;
						descriptor.shaderStage = shader.type;
						strcpy(descriptor.name, sampler.name);
						descriptor.type = SDT_SampledImage;
					}
					argument.m_vecShaderDescriptor.push_back(descriptor);
				}
				const StorageImage* stImage; // image1D 2D 3D/imageBuffer
				numImage = compiler->getStorageImages(&stImage);
				for (uint16_t i = 0; i < numImage; ++i) {
					const StorageImage& sampler = stImage[i];
					auto& argument = argumentLayouts[sampler.set];
					argument.m_vecStorageImage.push_back(sampler);
					Nix::ShaderDescriptor descriptor = {}; {
						descriptor.binding = sampler.binding;
						descriptor.dataSize = 0;
						descriptor.shaderStage = shader.type;
						strcpy(descriptor.name, sampler.name);
						descriptor.type = SDT_StorageImage;
					}
					argument.m_vecShaderDescriptor.push_back(descriptor);
				}
				const SubpassInput* subpassInputs;
				uint16_t numSubpass = compiler->getInputAttachment(&subpassInputs);
				for (uint16_t i = 0; i < numSubpass; ++i) {
					const SubpassInput& attachment = subpassInputs[i];
					auto& argument = argumentLayouts[attachment.set];
					argument.m_vecSubpassInput.push_back(attachment);
					Nix::ShaderDescriptor descriptor = {}; {
						descriptor.binding = attachment.binding;
						descriptor.dataSize = 0;
						descriptor.shaderStage = shader.type;
						strcpy(descriptor.name, attachment.name);
						descriptor.type = SDT_InputAttachment;
					}
					argument.m_vecShaderDescriptor.push_back(descriptor);
				}
			}
			else {
				break;
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
			if (vkpc.size) {
				constantRanges.push_back(vkpc);
			}
		}
		//
		std::vector<VkDescriptorSetLayoutBinding> dynamicBindings;
		std::vector<uint32_t> dynamicBindingTable;
		VkDescriptorSetLayout setLayouts[MaxArgumentCount];

		copyDescriptorSetLayoutToMaterial(argumentLayouts, materialDesc);

		for (uint32_t setIndex = 0; setIndex < materialDesc.argumentCount; ++setIndex) {
			std::vector<VkDescriptorSetLayoutBinding> bindings;
			//auto argument = argumentLayouts[setIndex];
			auto& argument = materialDesc.argumentLayouts[setIndex];
			for (uint32_t dscIndex = 0; dscIndex < argument.descriptorCount; ++dscIndex) {
				auto& descriptor = argument.descriptors[dscIndex];
				switch (descriptor.type) {
					//// ============================= image type =============================
				case SDT_Sampler: {
					VkDescriptorSetLayoutBinding binding;
					binding.binding = descriptor.binding;
					binding.descriptorCount = 1;
					binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
					binding.stageFlags = NixShaderStageToVk(descriptor.shaderStage);
					binding.pImmutableSamplers = nullptr;
					bindings.push_back(binding);
					break;
				}
				case SDT_SampledImage: {
					VkDescriptorSetLayoutBinding binding;
					binding.binding = descriptor.binding;
					binding.descriptorCount = 1;
					binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
					binding.stageFlags = NixShaderStageToVk(descriptor.shaderStage);
					binding.pImmutableSamplers = nullptr;
					bindings.push_back(binding);
					break;
				}
				case SDT_StorageImage: {
					VkDescriptorSetLayoutBinding binding;
					binding.binding = descriptor.binding;
					binding.descriptorCount = 1;
					binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
					binding.stageFlags = NixShaderStageToVk(descriptor.shaderStage);
					binding.pImmutableSamplers = nullptr;
					bindings.push_back(binding);
					break;
				}
				case SDT_SamplerImage: {
					VkDescriptorSetLayoutBinding binding;
					binding.binding = descriptor.binding;
					binding.descriptorCount = 1;
					binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
					binding.stageFlags = NixShaderStageToVk(descriptor.shaderStage);
					binding.pImmutableSamplers = nullptr;
					bindings.push_back(binding);
					break;
				}
				case SDT_TexelBuffer: {
					VkDescriptorSetLayoutBinding binding;
					binding.binding = descriptor.binding;
					binding.descriptorCount = 1;
					binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
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
										  //// ============================= buffer type =============================
				case SDT_UniformBuffer: {
					VkDescriptorSetLayoutBinding binding;
					binding.binding = descriptor.binding;
					binding.descriptorCount = 1;
					binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
					binding.stageFlags = NixShaderStageToVk(descriptor.shaderStage);
					binding.pImmutableSamplers = nullptr;
					bindings.push_back(binding);
					dynamicBindings.push_back(binding);
					break;
				}
				case SDT_StorageBuffer: {
					VkDescriptorSetLayoutBinding binding;
					binding.binding = descriptor.binding;
					binding.descriptorCount = 1;
					binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
					binding.stageFlags = NixShaderStageToVk(descriptor.shaderStage);
					binding.pImmutableSamplers = nullptr;
					bindings.push_back(binding);
					dynamicBindings.push_back(binding);
					break;
				}
				}
			}
			std::sort(dynamicBindings.begin(), dynamicBindings.end(), [](const VkDescriptorSetLayoutBinding& _b1, const VkDescriptorSetLayoutBinding& _b2) {
				return _b1.binding < _b2.binding;
			});

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
			for (size_t i = 0; i < dynamicBindings.size(); ++i) {
				if (dynamicBindingTable.size() < dynamicBindings[i].binding + 1) {
					dynamicBindingTable.resize(dynamicBindings[i].binding + 1);
				}
				dynamicBindingTable[dynamicBindings[i].binding] = i;
			}
			argumentLayouts[setIndex].m_dynamicOffsetIndexTable = dynamicBindingTable;
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
		material->m_topology = NixTopologyToVk(materialDesc.topologyMode);
		material->m_pologonMode = NixPolygonModeToVk(materialDesc.pologonMode);
		material->m_constantsStageFlags = constantsStageFlags;
		//
		return material;
	}

	VkShaderModule MaterialVk::CreateShaderModule(ContextVk* _context, const char * _shader, ShaderModuleType _type) {
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
			}
			else {
				auto module = NixCreateShaderModule(device, mem->constData(), mem->size() - 1);
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

	const UniformBuffer * ArgumentLayoutExt::getUniform(const std::string& _name, const std::vector<GLSLStructMember>*& _members) const {
		for (size_t i = 0; i < m_vecUniformBuffer.size(); ++i) {
			if (m_vecUniformBuffer[i].name == _name) {
				_members = &m_uniformLayouts[i];
				//auto it = m_offsetTable.find(m_vecUniformBuffer[i].binding);
				//assert(it != m_offsetTable.end());
				//_localOffset = it->second;
				return &m_vecUniformBuffer[i];
			}
		}
		return nullptr;
	}

	const StorageBuffer* ArgumentLayoutExt::getStorageBuffer(const std::string & _name) const {
		for (const auto& ssbo : m_vecStorageBuffer) {
			if (ssbo.name == _name) {
				return &ssbo;
			}
		}
		return nullptr;
	}

	const SeparateSampler* ArgumentLayoutExt::getSampler(const std::string & _name) const {
		for (const auto& sampler : m_vecSampler) {
			if (sampler.name == _name) {
				return &sampler;
			}
		}
		return nullptr;
	}

	const SeparateImage* ArgumentLayoutExt::getSampledImage(const std::string& _name) const {
		for (const auto& image : m_vecSampledImage) {
			if (image.name == _name) {
				return &image;
			}
		}
		return nullptr;
	}

	const StorageImage * ArgumentLayoutExt::getStorageImage(const std::string & _name) const {
		for (const auto& image : m_vecStorageImage) {
			if (image.name == _name) {
				return &image;
			}
		}
		return nullptr;
	}

	const CombinedImageSampler* ArgumentLayoutExt::getCombinedImageSampler(const std::string& _name) const {
		for (const auto& sampler : m_vecCombinedImageSampler) {
			if (sampler.name == _name) {
				return &sampler;
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
}
