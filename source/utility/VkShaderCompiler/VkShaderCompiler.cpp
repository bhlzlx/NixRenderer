#include "VkShaderCompiler.h"
#include <glslang/Public/ShaderLang.h>
#include <SPIRV/GlslangToSpv.h>
#include <StandAlone/DirStackFileIncluder.h>
#include <SPIRV-Cross/spirv_cross.hpp>
#include <algorithm>
#include <utility>

namespace Nix {
	//
	VkShaderCompiler compiler;
	//
	VkShaderCompiler* VkShaderCompiler::Instance = nullptr;
	//
	const TBuiltInResource DefaultTBuiltInResource = {
		/* .MaxLights = */ 32,
		/* .MaxClipPlanes = */ 6,
		/* .MaxTextureUnits = */ 32,
		/* .MaxTextureCoords = */ 32,
		/* .MaxVertexAttribs = */ 64,
		/* .MaxVertexUniformComponents = */ 4096,
		/* .MaxVaryingFloats = */ 64,
		/* .MaxVertexTextureImageUnits = */ 32,
		/* .MaxCombinedTextureImageUnits = */ 80,
		/* .MaxTextureImageUnits = */ 32,
		/* .MaxFragmentUniformComponents = */ 4096,
		/* .MaxDrawBuffers = */ 32,
		/* .MaxVertexUniformVectors = */ 128,
		/* .MaxVaryingVectors = */ 8,
		/* .MaxFragmentUniformVectors = */ 16,
		/* .MaxVertexOutputVectors = */ 16,
		/* .MaxFragmentInputVectors = */ 15,
		/* .MinProgramTexelOffset = */ -8,
		/* .MaxProgramTexelOffset = */ 7,
		/* .MaxClipDistances = */ 8,
		/* .MaxComputeWorkGroupCountX = */ 65535,
		/* .MaxComputeWorkGroupCountY = */ 65535,
		/* .MaxComputeWorkGroupCountZ = */ 65535,
		/* .MaxComputeWorkGroupSizeX = */ 1024,
		/* .MaxComputeWorkGroupSizeY = */ 1024,
		/* .MaxComputeWorkGroupSizeZ = */ 64,
		/* .MaxComputeUniformComponents = */ 1024,
		/* .MaxComputeTextureImageUnits = */ 16,
		/* .MaxComputeImageUniforms = */ 8,
		/* .MaxComputeAtomicCounters = */ 8,
		/* .MaxComputeAtomicCounterBuffers = */ 1,
		/* .MaxVaryingComponents = */ 60,
		/* .MaxVertexOutputComponents = */ 64,
		/* .MaxGeometryInputComponents = */ 64,
		/* .MaxGeometryOutputComponents = */ 128,
		/* .MaxFragmentInputComponents = */ 128,
		/* .MaxImageUnits = */ 8,
		/* .MaxCombinedImageUnitsAndFragmentOutputs = */ 8,
		/* .MaxCombinedShaderOutputResources = */ 8,
		/* .MaxImageSamples = */ 0,
		/* .MaxVertexImageUniforms = */ 0,
		/* .MaxTessControlImageUniforms = */ 0,
		/* .MaxTessEvaluationImageUniforms = */ 0,
		/* .MaxGeometryImageUniforms = */ 0,
		/* .MaxFragmentImageUniforms = */ 8,
		/* .MaxCombinedImageUniforms = */ 8,
		/* .MaxGeometryTextureImageUnits = */ 16,
		/* .MaxGeometryOutputVertices = */ 256,
		/* .MaxGeometryTotalOutputComponents = */ 1024,
		/* .MaxGeometryUniformComponents = */ 1024,
		/* .MaxGeometryVaryingComponents = */ 64,
		/* .MaxTessControlInputComponents = */ 128,
		/* .MaxTessControlOutputComponents = */ 128,
		/* .MaxTessControlTextureImageUnits = */ 16,
		/* .MaxTessControlUniformComponents = */ 1024,
		/* .MaxTessControlTotalOutputComponents = */ 4096,
		/* .MaxTessEvaluationInputComponents = */ 128,
		/* .MaxTessEvaluationOutputComponents = */ 128,
		/* .MaxTessEvaluationTextureImageUnits = */ 16,
		/* .MaxTessEvaluationUniformComponents = */ 1024,
		/* .MaxTessPatchComponents = */ 120,
		/* .MaxPatchVertices = */ 32,
		/* .MaxTessGenLevel = */ 64,
		/* .MaxViewports = */ 16,
		/* .MaxVertexAtomicCounters = */ 0,
		/* .MaxTessControlAtomicCounters = */ 0,
		/* .MaxTessEvaluationAtomicCounters = */ 0,
		/* .MaxGeometryAtomicCounters = */ 0,
		/* .MaxFragmentAtomicCounters = */ 8,
		/* .MaxCombinedAtomicCounters = */ 8,
		/* .MaxAtomicCounterBindings = */ 1,
		/* .MaxVertexAtomicCounterBuffers = */ 0,
		/* .MaxTessControlAtomicCounterBuffers = */ 0,
		/* .MaxTessEvaluationAtomicCounterBuffers = */ 0,
		/* .MaxGeometryAtomicCounterBuffers = */ 0,
		/* .MaxFragmentAtomicCounterBuffers = */ 1,
		/* .MaxCombinedAtomicCounterBuffers = */ 1,
		/* .MaxAtomicCounterBufferSize = */ 16384,
		/* .MaxTransformFeedbackBuffers = */ 4,
		/* .MaxTransformFeedbackInterleavedComponents = */ 64,
		/* .MaxCullDistances = */ 8,
		/* .MaxCombinedClipAndCullDistances = */ 8,
		/* .MaxSamples = */ 4,
		/* .maxMeshOutputVerticesNV*/ 256,
		/* .maxMeshOutputPrimitivesNV*/ 512,
		/* .maxMeshWorkGroupSizeX_NV*/ 32,
		/* .maxMeshWorkGroupSizeY_NV*/ 1,
		/* .maxMeshWorkGroupSizeZ_NV*/ 1,
		/* .maxTaskWorkGroupSizeX_NV*/ 32,
		/* .maxTaskWorkGroupSizeY_NV*/ 1,
		/* .maxTaskWorkGroupSizeZ_NV*/ 1,
		/* .maxMeshViewCountNV*/ 4,
		/* .limits = */
		{
			/* .nonInductiveForLoops = */ 1,
			/* .whileLoops = */ 1,
			/* .doWhileLoops = */ 1,
			/* .generalUniformIndexing = */ 1,
			/* .generalAttributeMatrixVectorIndexing = */ 1,
			/* .generalVaryingIndexing = */ 1,
			/* .generalSamplerIndexing = */ 1,
			/* .generalVariableIndexing = */ 1,
			/* .generalConstantMatrixVectorIndexing = */ 1,
		}
	};
	void VkShaderCompiler::addDescriptorRecord( uint16_t _id, uint16_t _binding ) {
		if (std::find(m_vecSetID.begin(), m_vecSetID.end(), _id) == m_vecSetID.end()) {
			m_vecSetID.push_back(_id);
		}
		auto& bindingRecord = m_bindingMap[_id];
		if (std::find(bindingRecord.begin(), bindingRecord.end(), _id) == bindingRecord.end()) {
			bindingRecord.push_back(_id);
		}
	}
	void VkShaderCompiler::clearResourceInfo()
	{
		m_vecSetID.clear();
		m_bindingMap.clear();
		m_UBOMemberInfo.clear();
		m_vecStageInput.clear();
		m_vecStageOutput.clear();
		m_pushConstants.offset = 0;
		m_pushConstants.size = 0;
		m_inputAttachment.clear();
		m_vecUBO.clear();
		m_vecSamplers.clear();
		m_vecSSBO.clear();
		m_vecTBO.clear();
		m_atomicCounter.clear();
	}
	//
	bool VkShaderCompiler::initializeEnvironment() {
		if (m_initailized) {
			return true;
		}
		bool rst = glslang::InitializeProcess();
		m_initailized = true;
		return rst;
	}

	void VkShaderCompiler::finalizeEnvironment() {
		glslang::FinalizeProcess();
	}
	bool VkShaderCompiler::compile(Nix::ShaderModuleType _type, const char * _text)
	{
		EShLanguage ShaderType = EShLanguage::EShLangVertex;
		switch (_type) {
		case Nix::VertexShader: ShaderType = EShLangVertex; break;
		case Nix::FragmentShader: ShaderType = EShLangFragment; break;
		case Nix::ComputeShader: ShaderType = EShLangCompute; break;
		}
		glslang::TShader Shader(ShaderType);

		Shader.setStrings(&_text, 1);

		int ClientInputSemanticsVersion = 100; // maps to, say, #define VULKAN 100
		glslang::EShTargetClientVersion VulkanClientVersion = glslang::EShTargetVulkan_1_0;  // would map to, say, Vulkan 1.0
		glslang::EShTargetLanguageVersion TargetVersion = glslang::EShTargetSpv_1_0;    // maps to, say, SPIR-V 1.0

		Shader.setEnvInput(glslang::EShSourceGlsl, ShaderType, glslang::EShClientVulkan, ClientInputSemanticsVersion);
		Shader.setEnvClient(glslang::EShClientVulkan, VulkanClientVersion);
		Shader.setEnvTarget(glslang::EShTargetSpv, TargetVersion);

		TBuiltInResource Resources;
		Resources = DefaultTBuiltInResource;
		EShMessages messages = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);

		const int DefaultVersion = 100;

		DirStackFileIncluder Includer;
		Includer.pushExternalLocalDirectory("");

		std::string PreprocessedGLSL;
		bool res = Shader.preprocess(&Resources, DefaultVersion, ENoProfile, false, false, messages, &PreprocessedGLSL, Includer);
		if (!res) {
			return false;
		}
		const char* PreprocessedCStr = PreprocessedGLSL.c_str();
		Shader.setStrings(&PreprocessedCStr, 1);

		res = Shader.parse(&Resources, 100, false, messages);
		if (!res) {
			const char* pLog = Shader.getInfoLog();
			const char* pDbgLog = Shader.getInfoDebugLog();
			m_compileMessage.clear();
			m_compileMessage += "Info :";
			m_compileMessage += pLog;
			m_compileMessage += "\r\n";
			m_compileMessage += "Debug :";
			m_compileMessage += pDbgLog;
			m_compileMessage += "\r\n";
			return false;
		}
		glslang::TProgram Program;
		Program.addShader(&Shader);
		res = Program.link(messages);
		assert(res == true);
		spv::SpvBuildLogger logger;
		glslang::SpvOptions spvOptions;
		m_compiledSPV.clear();
		glslang::GlslangToSpv(*Program.getIntermediate(ShaderType), m_compiledSPV, &logger, &spvOptions);
		return true;
	}

	bool VkShaderCompiler::getCompiledSpv(const uint32_t*& _spv, size_t& _numU32) const {
		if (!m_compiledSPV.size()) {
			return false;
		}
		_spv = m_compiledSPV.data();
		_numU32 = m_compiledSPV.size();
		return true;
	}

	//>! 
	bool VkShaderCompiler::parseSpvLayout(Nix::ShaderModuleType _type, uint32_t * _spv, size_t _numU32) {
		spirv_cross::Compiler spvCompiler(_spv, _numU32);
		spirv_cross::ShaderResources spvResource = spvCompiler.get_shader_resources();
		// get vertex binding information
		m_vecStageInput.clear();
		for (auto& vertexInput : spvResource.stage_inputs) {
			auto location = spvCompiler.get_decoration(vertexInput.id, spv::Decoration::DecorationLocation);
			StageIOAttribute attr; 
			attr.location = location;
			attr.name = vertexInput.name;
			//attr.type = vertexInput.base_type_id;
			m_vecStageInput[location] = attr;
			//
		}
		m_vecStageOutput.clear();
		for (auto& vertexInput : spvResource.stage_inputs) {
			auto location = spvCompiler.get_decoration(vertexInput.id, spv::Decoration::DecorationLocation);
			StageIOAttribute attr;
			attr.location = location;
			attr.name = vertexInput.name;
			//attr.type = vertexInput.base_type_id;
			m_vecStageOutput[location] = attr;
		}
		//
		if (spvResource.push_constant_buffers.size()) {
			auto& constants = spvResource.push_constant_buffers[0];
			auto ranges = spvCompiler.get_active_buffer_ranges(constants.id);
			m_pushConstants.offset = ranges[0].offset;
			m_pushConstants.size = (ranges.back().offset - m_pushConstants.offset) + ranges.back().range;
			m_pushConstants.offset = m_pushConstants.offset;
		}
		//
		for (auto& pass : spvResource.subpass_inputs) {
			SubpassInput input;
			input.set = spvCompiler.get_decoration(pass.id, spv::Decoration::DecorationDescriptorSet);
			input.binding = spvCompiler.get_decoration(pass.id, spv::Decoration::DecorationBinding);
			input.inputIndex = spvCompiler.get_decoration(pass.id, spv::Decoration::DecorationIndex);
			m_inputAttachment.push_back(input);
			addDescriptorRecord(input.set, input.binding);
		}
		//
		for (auto& uniformBlock : spvResource.uniform_buffers) {
			UniformBlock block;
			block.set = spvCompiler.get_decoration(uniformBlock.id, spv::Decoration::DecorationDescriptorSet);
			block.binding = spvCompiler.get_decoration(uniformBlock.id, spv::Decoration::DecorationBinding);
			block.name = uniformBlock.name;
			auto uniformType = spvCompiler.get_type_from_variable(uniformBlock.id);
			block.size = spvCompiler.get_declared_struct_size(uniformType);
			m_vecUBO.push_back(block);
			addDescriptorRecord(block.set, block.binding);
			//
			DescriptorKey key(block.set, block.binding);
			auto& layoutRecord = m_UBOMemberInfo[key.val];
			uint32_t memberIndex = 0;
			while (true) {
				std::string name = spvCompiler.get_member_name(uniformBlock.base_type_id, memberIndex);
				if (!name.length()) {
					break;
				}
				UniformBlock::Member member;
				member.name = name;
				member.offset = spvCompiler.get_member_decoration(uniformBlock.base_type_id, memberIndex, spv::Decoration::DecorationOffset);
				//
				layoutRecord.push_back(member);
				++memberIndex;
			}
			std::sort(layoutRecord.begin(), layoutRecord.end(), []( const UniformBlock::Member& _member1, const UniformBlock::Member& _member2 ) {
				return _member1.offset < _member2.offset;
			});
			uint32_t prevMemberOffset = -1;
			for (int memberIdx = 0; memberIdx < (int)layoutRecord.size() - 1; memberIdx++) {
				prevMemberOffset = layoutRecord[memberIdx].offset;
				layoutRecord[memberIdx].size = layoutRecord[memberIdx + 1].offset - layoutRecord[memberIdx].offset;
			}
			if (prevMemberOffset != -1) {
				layoutRecord.back().size = block.size - prevMemberOffset;
			}
			else {
				layoutRecord.back().size = block.size;
			}
		}
		//>!!!!
		for (auto& sampler : spvResource.sampled_images) {
			CombinedImageSampler image;
			image.binding = spvCompiler.get_decoration(sampler.id, spv::Decoration::DecorationBinding);
			image.set = spvCompiler.get_decoration(sampler.id, spv::Decoration::DecorationDescriptorSet);
			image.name = sampler.name;
			m_vecSamplers.push_back(image);
			addDescriptorRecord(image.set, image.binding);
		}
		//
		m_vecSSBO.clear();
		for (auto& item : spvResource.storage_buffers) {
			ShaderStorageBufferObject ssbo;
			ssbo.binding = spvCompiler.get_decoration(item.id, spv::Decoration::DecorationBinding);
			ssbo.set = spvCompiler.get_decoration(item.id, spv::Decoration::DecorationDescriptorSet);
			ssbo.name = item.name;
			auto uniformType = spvCompiler.get_type_from_variable(item.id);
			ssbo.size = spvCompiler.get_declared_struct_size(uniformType);
			m_vecSSBO.push_back(ssbo);
			addDescriptorRecord(ssbo.set, ssbo.binding);
		}
		//
		m_vecTBO.clear();
		for (auto& item : spvResource.storage_images) {
			TexelBufferObject tbo;
			tbo.binding = spvCompiler.get_decoration(item.id, spv::Decoration::DecorationBinding);
			tbo.set = spvCompiler.get_decoration(item.id, spv::Decoration::DecorationDescriptorSet);
			tbo.name = item.name;
			m_vecTBO.push_back(tbo);
			addDescriptorRecord(tbo.set, tbo.binding);
		}
		m_atomicCounter.clear();
		for (auto& item : spvResource.atomic_counters) {
			AtomicCounter counter;
			counter.binding = spvCompiler.get_decoration(item.id, spv::Decoration::DecorationBinding);
			counter.set = spvCompiler.get_decoration(item.id, spv::Decoration::DecorationDescriptorSet);
			counter.name = item.name;
			m_atomicCounter.push_back(counter);
			addDescriptorRecord(counter.set, counter.binding);
		}
		return true;
	}
}

extern "C" {
	Nix::VkShaderCompiler * GetVkCompiler() {
		return &Nix::compiler;
	}
}
