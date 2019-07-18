#include "MaterialVk.h"
#include "ContextVk.h"
#include "RenderableVk.h"
#include "DescriptorSetVk.h"
#include "TypemappingVk.h"
#include "ContextVk.h"
#include "DriverVk.h"

namespace Nix {

	bool MaterialVk::ValidationShaderDescriptor( ShaderDescriptor& _descriptor, const uint32_t _setIndex, const spirv_cross::Compiler& _compiler, const spirv_cross::ShaderResources& _resources, ContextVk* _context, spirv_cross::Resource& res ) {
		if (_descriptor.type == SDT_UniformBlock) 
		{
			for (auto& shaderRes : _resources.uniform_buffers) 
			{
				// find a shader descriptor with a same name
				if (shaderRes.name == _descriptor.name)
				{
					auto set = _compiler.get_decoration( shaderRes.id, spv::Decoration::DecorationDescriptorSet);
					auto binding = _compiler.get_decoration(shaderRes.id, spv::Decoration::DecorationBinding);
					if (set == _setIndex && binding == _descriptor.binding) 
					{
						auto uniformType = _compiler.get_type_from_variable(shaderRes.id);
						auto uniformChunkSize = _compiler.get_declared_struct_size(uniformType);
						_descriptor.dataSize = (uint32_t)uniformChunkSize;
						res = shaderRes;
						return true;
					}
				}
			}
			return false;
		}
		else if (_descriptor.type == SDT_Sampler) 
		{
			for (auto& shaderRes : _resources.sampled_images)
			{
				// find a shader descriptor with a same name
				if (shaderRes.name == _descriptor.name)
				{
					auto set = _compiler.get_decoration(shaderRes.id, spv::Decoration::DecorationDescriptorSet);
					auto binding = _compiler.get_decoration(shaderRes.id, spv::Decoration::DecorationBinding);
					if (set == _setIndex && binding == _descriptor.binding)
					{
						res = shaderRes;
						return true;
					}
				}
			}
			return false;
		}
		else if (_descriptor.type == SDT_SSBO)
		{
			for (auto& shaderRes : _resources.storage_buffers)
			{
				// find a shader descriptor with a same name
				if (shaderRes.name == _descriptor.name)
				{
					auto set = _compiler.get_decoration(shaderRes.id, spv::Decoration::DecorationDescriptorSet);
					auto binding = _compiler.get_decoration(shaderRes.id, spv::Decoration::DecorationBinding);
					if (set == _setIndex && binding == _descriptor.binding)
					{
						return true;
					}
				}
			}
			return false;
		}
		return true;
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
		VkDevice device = _context->getDevice();
		// load SPV from disk!
		std::vector<uint32_t> vertexSpvBytes;
		std::vector<uint32_t> fragmentSpvBytes;
		VkShaderModule vertSM = CreateShaderModule(_context, _desc.vertexShader, VertexShader, vertexSpvBytes);
		if (VK_NULL_HANDLE == vertSM) {
			assert(false); return nullptr;
		}
		VkShaderModule fragSM = CreateShaderModule(_context, _desc.fragmentShader, FragmentShader, fragmentSpvBytes);
		if (VK_NULL_HANDLE == fragSM) {
			assert(false); return nullptr;
		}
		// reflect the resource information
		spirv_cross::Compiler vertCompiler(vertexSpvBytes.data(), vertexSpvBytes.size());
		spirv_cross::ShaderResources vertexResource = vertCompiler.get_shader_resources();
		spirv_cross::Compiler fragCompiler(fragmentSpvBytes.data(), fragmentSpvBytes.size());
		spirv_cross::ShaderResources fragmentResource = fragCompiler.get_shader_resources();
		// 1. verify the vertex input layout
		// 2. verify the descriptor set description
		// 3. get the push constants information

		//\ 1 - vertex layout validation
		if (materialDesc.vertexLayout.attributeCount != vertexResource.stage_inputs.size()) {
			_context->getDriver()->getLogger()->error("vertex attribute count mismatch! ");
			return nullptr;
		}
		for (auto& vertexInput : vertexResource.stage_inputs) {
			auto location = vertCompiler.get_decoration(vertexInput.id, spv::Decoration::DecorationLocation);
			if (materialDesc.vertexLayout.attributes[location].name != vertexInput.name) {
				assert("name does not match!" && false);
				return nullptr;
			}
		}
		//\ 2 - descriptor set validation

		spirv_cross::Compiler* compiler = nullptr;
		spirv_cross::ShaderResources* shaderResource = nullptr;

		std::array<ArgumentLayout, MaxArgumentCount> argumentLayouts;

		for (uint32_t argumentIndex = 0; argumentIndex < materialDesc.argumentCount; ++argumentIndex) {
			auto& argument = materialDesc.argumentLayouts[argumentIndex];
			for (uint32_t descriptorIndex = 0; descriptorIndex < argument.descriptorCount; ++descriptorIndex) {
				auto& descinfo = argument.descriptors[descriptorIndex];

				if (descinfo.shaderStage == VertexShader) {
					compiler = &vertCompiler; shaderResource = &vertexResource;
				}
				else if (descinfo.shaderStage == FragmentShader) {
					compiler = &fragCompiler; shaderResource = &fragmentResource;
				}
				spirv_cross::Resource resItem;
				if (!ValidationShaderDescriptor(descinfo, argument.index, *compiler, *shaderResource, _context, resItem))
				{
					_context->getDriver()->getLogger()->error("shader descriptor set mismatch! ");
					_context->getDriver()->getLogger()->error("name : ");
					_context->getDriver()->getLogger()->error(descinfo.name);
					_context->getDriver()->getLogger()->error("\n");
					return nullptr;
				}
				// collect uniform block member information
				if (descinfo.type == SDT_UniformBlock) {
					uint32_t memberIndex = 0;
					while (true) {
						std::string name = compiler->get_member_name(resItem.base_type_id, memberIndex);
						if (!name.length()) {
							break;
						}
						ArgumentLayout::UniformMember member;
						member.name = name;
						member.offset = compiler->get_member_decoration(resItem.base_type_id, memberIndex, spv::Decoration::DecorationOffset);
						member.binding = descinfo.binding;
						argumentLayouts[argumentIndex].m_uniformMembers.push_back(member);
						++memberIndex;
					}
				}

				switch (descinfo.type) {
				case SDT_Sampler:
					argumentLayouts[argumentIndex].m_samplerImageDescriptor.push_back(descinfo); break;
				case SDT_UniformBlock:
					argumentLayouts[argumentIndex].m_uniformBlockDescriptor.push_back(descinfo); break;
				case SDT_SSBO:
					argumentLayouts[argumentIndex].m_storageBufferDescriptor.push_back(descinfo); break;
				case SDT_TBO:
					argumentLayouts[argumentIndex].m_texelBufferDescriptor.push_back(descinfo); break;
				}

				if (descinfo.type == SDT_UniformBlock) {
					uint32_t i = 0;
					while (true) {
						const auto & name = compiler->get_member_name(resItem.type_id, i);
						if (name.empty()) {
							break;
						}
						else 
						{
							auto offset = compiler->get_member_decoration(resItem.type_id, i, spv::Decoration::DecorationOffset);
							argumentLayouts[argumentIndex].m_uniformMembers.push_back({
								name, descinfo.binding, offset
							});
						}
					}
				}
			}
		}

		//\ 3 - push constants information
		std::vector< VkPushConstantRange > constantRanges;

		if (vertexResource.push_constant_buffers.size()) {
			auto& vc = vertexResource.push_constant_buffers[0];
			auto ranges = vertCompiler.get_active_buffer_ranges(vc.id);
			VkPushConstantRange range = {};
			range.offset = (uint32_t)ranges[0].offset;
			range.size = (uint32_t)ranges[0].range;
			range.stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT;
			constantRanges.push_back(range);
		}

		if (fragmentResource.push_constant_buffers.size()) {
			auto& fc = fragmentResource.push_constant_buffers[0];
			auto ranges = vertCompiler.get_active_buffer_ranges(fc.id);
			VkPushConstantRange range = {};
			range.offset = (uint32_t)ranges[0].offset;
			range.size = (uint32_t)ranges[0].range;
			range.stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT;
			constantRanges.push_back(range);
		}
		//
		VkDescriptorSetLayout layouts[MaxArgumentCount];
		for (uint32_t layoutIndex = 0; layoutIndex < materialDesc.argumentCount; ++layoutIndex) {
			std::vector<VkDescriptorSetLayoutBinding> bindings;
			auto& argument = materialDesc.argumentLayouts[layoutIndex];
            for( uint32_t dscIndex = 0; dscIndex < argument.descriptorCount; ++dscIndex){
                auto& descriptor = argument.descriptors[dscIndex];
                if (descriptor.type == SDT_UniformBlock)
				{
					VkDescriptorSetLayoutBinding binding;
					binding.binding = descriptor.binding;
					binding.descriptorCount = 1;
					binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
					binding.stageFlags = NixShaderStageToVk(descriptor.shaderStage);
					binding.pImmutableSamplers = nullptr;
					bindings.push_back(binding);
				}
				else if (descriptor.type == SDT_Sampler)
				{
					VkDescriptorSetLayoutBinding binding;
					binding.binding = descriptor.binding;
					binding.descriptorCount = 1;
					binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
					binding.stageFlags = NixShaderStageToVk(descriptor.shaderStage);
					binding.pImmutableSamplers = nullptr;
					bindings.push_back(binding);
				}
				else if (descriptor.type == SDT_SSBO)
				{
					VkDescriptorSetLayoutBinding binding;
					binding.binding = descriptor.binding;
					binding.descriptorCount = 1;
					binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
					binding.stageFlags = NixShaderStageToVk(descriptor.shaderStage);
					binding.pImmutableSamplers = nullptr;
					bindings.push_back(binding);
				}
            }

			VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {}; {
				layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
				layoutCreateInfo.pNext = nullptr;
				layoutCreateInfo.flags = 0;
				layoutCreateInfo.bindingCount = (uint32_t)bindings.size();
				layoutCreateInfo.pBindings = bindings.data();
			}
			vkCreateDescriptorSetLayout(device, &layoutCreateInfo, nullptr, &layouts[layoutIndex]);
			argumentLayouts[layoutIndex].m_descriptorSetIndex = layoutIndex;
			argumentLayouts[layoutIndex].m_descriptorSetLayout = layouts[layoutIndex];
			argumentLayouts[layoutIndex].updateDynamicalBindings();
		}
		// create pipeline layout
		VkPipelineLayoutCreateInfo info; {
			info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			info.pNext = nullptr;
			info.flags = 0;
			info.pSetLayouts = layouts;
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
		material->m_vertexShader = vertSM;
		material->m_fragmentShader = fragSM;
		material->m_descriptorSetLayoutCount = (uint32_t)materialDesc.argumentCount;
		material->m_pipelineLayout = pipelineLayout;
		material->m_argumentLayouts = argumentLayouts;
		material->m_topology = NixTopologyToVk( materialDesc.topologyMode );
		material->m_pologonMode = NixPolygonModeToVk(materialDesc.pologonMode);
		//
		return material;
	}

	const ShaderDescriptor* ArgumentLayout::getUniformBlock(const std::string& _name)
	{
		uint32_t i = 0;
		for (; i < m_uniformBlockDescriptor.size(); ++i) {
			if (m_uniformBlockDescriptor.at(i).type == SDT_UniformBlock) {
				if (m_uniformBlockDescriptor.at(i).name == _name) {
					return &m_uniformBlockDescriptor.at(i);
				}
			}
		}
		return nullptr;
	}

	uint32_t ArgumentLayout::getUniformBlockMemberOffset(uint32_t _binding, const std::string& _name)
	{
		for (auto& member : m_uniformMembers) {
			if (member.binding == _binding && member.name == _name ) {
				return member.offset;
			}
		}
		return -1;
	}

	const ShaderDescriptor* ArgumentLayout::getSampler(const std::string& _name)
	{
		uint32_t i = 0;
		for (; i < m_samplerImageDescriptor.size(); ++i) {
			if (m_samplerImageDescriptor[i].type == SDT_Sampler) {
				if (m_samplerImageDescriptor[i].name == _name) {
					return &m_samplerImageDescriptor[i];
				}
			}
		}
		return nullptr;
	}

	const ShaderDescriptor* ArgumentLayout::getSSBO(const std::string& _name)
	{
		uint32_t i = 0;
		for (; i < m_storageBufferDescriptor.size(); ++i) {
			if (m_storageBufferDescriptor[i].type == SDT_SSBO) {
				if (m_storageBufferDescriptor[i].name == _name) {
					return &m_storageBufferDescriptor[i];
				}
			}
		}
		return nullptr;
	}

	void ArgumentLayout::updateDynamicalBindings()
	{
		// only uniform buffer object need dynamical bindings
		for (auto& descriptor : m_uniformBlockDescriptor) {
			m_dynamicalBindings.push_back(descriptor.binding);
		}
		//
		std::sort(m_dynamicalBindings.begin(), m_dynamicalBindings.end());
	}

	VkShaderModule MaterialVk::CreateShaderModule(ContextVk* _context, const char * _shader, ShaderModuleType _type, std::vector<uint32_t>& _spvBytes)
	{
		auto length = strlen(_shader);

		const char* compileInfo;
		const uint32_t* spvBytes;
		size_t spvLength;
		// 
		if (length < 96 && length > 4) { // if @_shader is a file path
			VkDevice device = _context->getDevice();
			// load SPV from disk!
			auto arch = _context->getDriver()->getArchieve();
			Nix::IFile * file = arch->open(VULKAN_SHADER_PATH(_shader));
			if (!file) { assert(false); return nullptr; }
			Nix::IFile* mem = CreateMemoryBuffer(file->size() + 1);
			mem->seek(SeekEnd, -1);
			char endle = 0;
			mem->write(1, &endle);
			mem->seek(SeekSet, 0);
			file->read(file->size(), mem);
			file->release();
			if (*(const char*)mem->constData() == '#') {
				DriverVk* driver = (DriverVk*)_context->getDriver();
				bool rst = driver->compileGLSL2SPV(_type, (const char*)mem->constData(), &spvBytes, &spvLength, &compileInfo);
				if (!rst) {
					mem->release();
					return VK_NULL_HANDLE;
				}
				_spvBytes = std::vector<uint32_t>(spvBytes, spvBytes + spvLength);
				VkShaderModule module = NixCreateShaderModule(device, spvBytes, spvLength * sizeof(uint32_t));
				return module;
			} else {
				auto module = NixCreateShaderModule(device, mem->constData(), mem->size() - 1 );
				_spvBytes = std::vector<uint32_t>((uint32_t*)mem->constData(), (uint32_t*)mem->constData() + mem->size() / sizeof(uint32_t));
				mem->release();
				return module;
			}
		}
		else
		{
			if (*_shader == '#') { // @_shader is shader text content
				DriverVk* driver = (DriverVk*)_context->getDriver();
				bool rst = driver->compileGLSL2SPV(_type, _shader, &spvBytes, &spvLength, &compileInfo);
				if (!rst) {
					return VK_NULL_HANDLE;
				}
				_spvBytes = std::vector<uint32_t>(spvBytes, spvBytes + spvLength);
				return NixCreateShaderModule( _context->getDevice(), spvBytes, spvLength * sizeof(uint32_t));
			}
		}
		return VK_NULL_HANDLE;
	}

}