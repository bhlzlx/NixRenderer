#include "VkShaderCompiler.h"
#include <glslang/Public/ShaderLang.h>
#include <SPIRV/GlslangToSpv.h>
#include <StandAlone/DirStackFileIncluder.h>
#include <SPIRV-Cross/spirv_cross.hpp>
#include <SPIRV-Cross/spirv_parser.hpp>
#include <algorithm>
#include <utility>

extern "C" {
	Nix::spvcompiler::IShaderCompiler* compilerInstance = nullptr;
}

namespace Nix {

	namespace spvcompiler {

		class VkShaderCompiler
			: public IShaderCompiler
			, public spirv_cross::Compiler
		{
		private:
			uint32_t										m_numRef;
			//
			std::vector<uint32_t>							m_compiledSPV;		// binary spv data
			std::string										m_compileMessage;	// compiling error/warnings
																				//
			std::vector<uint16_t>							m_vecSetID;			// stores the set id
			std::map<uint16_t, std::vector<uint16_t>>		m_bindingMap;		// set as key, binding as value
			std::map < uint32_t, std::vector<GLSLStructMember>>
				m_UBOMemberInfo;
			std::vector<StageIOAttribute>					m_vecStageInput;
			std::vector<StageIOAttribute>					m_vecStageOutput;
			PushConstants									m_pushConstants;
			//
			std::vector<UniformBuffer>						m_vecUBO;
			std::vector<StorageBuffer>						m_vecStorageBuffer;
			//
			std::vector<SubpassInput>						m_vecInputAttachment;
			std::vector<SeparateSampler>					m_vecSamplers;
			std::vector<SeparateImage>						m_vecSampledImages;
			std::vector<StorageImage>						m_vecStorageImages;
			std::vector<CombinedImageSampler>				m_vecCombinedImageSampler;
			// deprecated types
			//std::vector<TexelBufferObject>					m_vecTBO;
			//std::vector<AtomicCounter>						m_atomicCounter;
		private:
			static VkShaderCompiler* Instance;
			bool											m_initailized;
			//
			void addDescriptorRecord(uint16_t _id, uint16_t _binding);
			void clearResourceInfo();
		public:
			VkShaderCompiler() : Compiler(nullptr, 0) {
				assert(!Instance);
				m_numRef = 0;
				Instance = this;
				m_initailized = false;
			}

			void retain() {
				++m_numRef;
			}
			void release() {
				--m_numRef;
				if (!m_numRef) {
					finalizeEnvironment();
					delete this;
					compilerInstance = nullptr;
				}
			}
			// 初始化/清理 生产环境
			bool initializeEnvironment();
			void finalizeEnvironment();
			// 将 GLSL 编译成 SPV
			bool compile(Nix::ShaderModuleType _type, const char* _text);
			bool getCompiledSpv(const uint32_t** _spv, size_t* _numU32) const;
			// 解析SPV
			bool parseSpvLayout(Nix::ShaderModuleType _type, const uint32_t* _spv, size_t _numU32);
			// 获取信息
			uint16_t getDescriptorSetCount() {
				return (uint16_t)m_vecSetID.size();
			}

			uint16_t getDescriptorSets(const uint16_t** _sets) const {
				uint16_t c = m_vecSetID.size();
				if (c) {
					*_sets = m_vecSetID.data();
				}
				return c;
			}
			uint16_t getBindings(uint16_t _setID, const uint16_t** _bindings) {
				auto it = m_bindingMap.find(_setID);
				if (it == m_bindingMap.end()) {
					return 0;
				}
				uint16_t count = it->second.size();
				*_bindings = it->second.data();
				return count;
			}
			uint16_t getUniformBuffers(const UniformBuffer** _blocks) {
				uint16_t count = m_vecUBO.size();
				if (count) {
					*_blocks = m_vecUBO.data();
				}
				return count;
			}
			uint16_t getUniformBufferMemebers(uint16_t _set, uint16_t _binding, const GLSLStructMember** _member) {
				DescriptorKey key(_set, _binding);
				auto it = m_UBOMemberInfo.find(key.val);
				if (it == m_UBOMemberInfo.end()) {
					return 0;
				}
				*_member = it->second.data();
				return it->second.size();
			}
			void getPushConstants(uint16_t* _offset, uint16_t* _size) {
				*_offset = m_pushConstants.offset;
				*_size = m_pushConstants.size;
			}
			uint16_t getInputAttachment(const SubpassInput** _attachments) {
				if (m_vecInputAttachment.size()) {
					*_attachments = m_vecInputAttachment.data();
				}
				return m_vecInputAttachment.size();
			}
			uint16_t getStageInput(const StageIOAttribute** _inputs) {
				if (m_vecStageInput.size()) {
					*_inputs = m_vecStageInput.data();
				}
				return m_vecStageInput.size();
			}
			uint16_t getStageOutput(const StageIOAttribute** _outputs) {
				if (m_vecStageOutput.size()) {
					*_outputs = m_vecStageOutput.data();
				}
				return m_vecStageOutput.size();
			}
			uint16_t getShaderStorageBuffers(const StorageBuffer** _ssbo) {
				if (m_vecStorageBuffer.size()) {
					*_ssbo = m_vecStorageBuffer.data();
				}
				return m_vecStorageBuffer.size();
			}
			uint16_t getSamplers(const SeparateSampler** _samplers) {
				if (m_vecSamplers.size()) {
					*_samplers = m_vecSamplers.data();
				}
				return m_vecSamplers.size();
			}
			uint16_t getStorageImages(const StorageImage** _images) {
				if (m_vecStorageImages.size()) {
					*_images = m_vecStorageImages.data();
				}
				return m_vecStorageImages.size();
			}
			uint16_t getSampledImages(const SeparateImage** _images) {
				if (m_vecSampledImages.size()) {
					*_images = m_vecSampledImages.data();
				}
				return m_vecSampledImages.size();
			}
			uint16_t getCombinedImageSampler(const CombinedImageSampler** _images) {
				if (m_vecCombinedImageSampler.size()) {
					*_images = m_vecCombinedImageSampler.data();
				}
				return m_vecCombinedImageSampler.size();
			}

			Nix::VertexType getVertexType(const spirv_cross::SPIRType& _type) {
				switch (_type.basetype) {
				case spirv_cross::SPIRType::BaseType::Float: {
					Nix::VertexType types[] = {
						Nix::VertexType::VertexTypeFloat,
						Nix::VertexType::VertexTypeFloat,
						Nix::VertexType::VertexTypeFloat2,
						Nix::VertexType::VertexTypeFloat3,
					};
					return types[_type.vecsize];
				}
				case spirv_cross::SPIRType::BaseType::Int: {
					Nix::VertexType types[] = {
						Nix::VertexType::VertexTypeUint,
						Nix::VertexType::VertexTypeUint2,
						Nix::VertexType::VertexTypeUint3,
						Nix::VertexType::VertexTypeUint4,
					};
					return types[_type.vecsize];
				}
				case spirv_cross::SPIRType::BaseType::UInt: {
					Nix::VertexType types[] = {
						Nix::VertexType::VertexTypeUint,
						Nix::VertexType::VertexTypeUint2,
						Nix::VertexType::VertexTypeUint3,
						Nix::VertexType::VertexTypeUint4,
					};
					return types[_type.vecsize];
				}
				case spirv_cross::SPIRType::BaseType::UByte: {
					Nix::VertexType types[] = {
						Nix::VertexType::VertexTypeUByte4,
					};
					return types[_type.vecsize];
				}
				case spirv_cross::SPIRType::BaseType::Boolean: {
				}
				}
				return VertexTypeInvalid;
			}
		};
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
		void VkShaderCompiler::addDescriptorRecord(uint16_t _id, uint16_t _binding) {
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
			m_vecInputAttachment.clear();
			m_vecUBO.clear();
			m_vecSamplers.clear();
			m_vecStorageBuffer.clear();
			m_vecStorageImages.clear();
			m_vecSampledImages.clear();
			//m_atomicCounter.clear();
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

		bool VkShaderCompiler::getCompiledSpv(const uint32_t** _spv, size_t* _numU32) const {
			if (!m_compiledSPV.size()) {
				return false;
			}
			*_spv = m_compiledSPV.data();
			*_numU32 = m_compiledSPV.size();
			return true;
		}

		//>! 
		bool VkShaderCompiler::parseSpvLayout(Nix::ShaderModuleType _type, const uint32_t * _spv, size_t _numU32) {
			//spirv_cross::Compiler spvCompiler(_spv, _numU32);
			spirv_cross::Parser parser(_spv, _numU32);
			parser.parse();
			set_ir(parser.get_parsed_ir());
			//
			spirv_cross::ShaderResources spvResource = get_shader_resources();
			// get vertex binding information
			m_vecStageInput.clear();
			for (auto& stageInput : spvResource.stage_inputs) {
				auto location = get_decoration(stageInput.id, spv::Decoration::DecorationLocation);
				StageIOAttribute attr;
				attr.location = location;
				strcpy(attr.name, stageInput.name.c_str());
				//
				auto& type = get<spirv_cross::SPIRType>(stageInput.base_type_id);
				attr.type = getVertexType(type);
				m_vecStageInput.push_back(attr);
			}
			m_vecStageOutput.clear();
			for (auto& stageOutput : spvResource.stage_outputs) {
				auto location = get_decoration(stageOutput.id, spv::Decoration::DecorationLocation);
				StageIOAttribute attr;
				attr.location = location;
				strcpy(attr.name, stageOutput.name.c_str());
				auto& type = get<spirv_cross::SPIRType>(stageOutput.base_type_id);
				attr.type = getVertexType(type);
				m_vecStageOutput.push_back(attr);
			}
			//
			m_pushConstants.offset = 0;
			m_pushConstants.size = 0;
			//
			if (spvResource.push_constant_buffers.size()) {
				auto& constants = spvResource.push_constant_buffers[0];
				auto ranges = get_active_buffer_ranges(constants.id);
				m_pushConstants.offset = ranges[0].offset;
				m_pushConstants.size = (ranges.back().offset - m_pushConstants.offset) + ranges.back().range;
				m_pushConstants.offset = m_pushConstants.offset;
			}
			// ======================= buffer type ===========================
			for (auto& uniformBlock : spvResource.uniform_buffers) {
				UniformBuffer block;
				block.set = get_decoration(uniformBlock.id, spv::Decoration::DecorationDescriptorSet);
				block.binding = get_decoration(uniformBlock.id, spv::Decoration::DecorationBinding);
				strcpy(block.name, uniformBlock.name.c_str());
				auto uniformType = get_type_from_variable(uniformBlock.id);
				block.size = get_declared_struct_size(uniformType);
				m_vecUBO.push_back(block);
				addDescriptorRecord(block.set, block.binding);
				//
				DescriptorKey key(block.set, block.binding);
				auto& layoutRecord = m_UBOMemberInfo[key.val];
				uint32_t memberIndex = 0;
				while (true) {
					std::string name = get_member_name(uniformBlock.base_type_id, memberIndex);
					if (!name.length()) {
						break;
					}
					GLSLStructMember member;
					strcpy(member.name, name.c_str());
					member.offset = get_member_decoration(uniformBlock.base_type_id, memberIndex, spv::Decoration::DecorationOffset);
					//
					layoutRecord.push_back(member);
					++memberIndex;
				}
				std::sort(layoutRecord.begin(), layoutRecord.end(), [](const GLSLStructMember& _member1, const GLSLStructMember& _member2) {
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
			//
			m_vecStorageBuffer.clear();
			for (auto& item : spvResource.storage_buffers) {
				StorageBuffer ssbo;
				ssbo.binding = get_decoration(item.id, spv::Decoration::DecorationBinding);
				ssbo.set = get_decoration(item.id, spv::Decoration::DecorationDescriptorSet);
				strcpy(ssbo.name, item.name.c_str());
				auto uniformType = get_type_from_variable(item.id);
				ssbo.size = get_declared_struct_size(uniformType);
				m_vecStorageBuffer.push_back(ssbo);
				addDescriptorRecord(ssbo.set, ssbo.binding);
			}
			// ======================= image type ===========================
			// 4 type : sampler/sampled image/storage image/input attachment
			for (auto& sampler : spvResource.separate_samplers) {
				SeparateSampler sam;
				sam.binding = get_decoration(sampler.id, spv::Decoration::DecorationBinding);
				sam.set = get_decoration(sampler.id, spv::Decoration::DecorationDescriptorSet);
				strcpy(sam.name, sampler.name.c_str());
				m_vecSamplers.push_back(sam);
				addDescriptorRecord(sam.set, sam.binding);
			}
			for (auto& image : spvResource.separate_images) {
				SeparateImage img;
				img.binding = get_decoration(image.id, spv::Decoration::DecorationBinding);
				img.set = get_decoration(image.id, spv::Decoration::DecorationDescriptorSet);
				strcpy(img.name, image.name.c_str());
				m_vecSampledImages.push_back(img);
				addDescriptorRecord(img.set, img.binding);
			}
			for (auto& sampledImage : spvResource.sampled_images) {
				CombinedImageSampler image;
				image.binding = get_decoration(sampledImage.id, spv::Decoration::DecorationBinding);
				image.set = get_decoration(sampledImage.id, spv::Decoration::DecorationDescriptorSet);
				strcpy(image.name, sampledImage.name.c_str());
				m_vecCombinedImageSampler.push_back(image);
				addDescriptorRecord(image.set, image.binding);
			}
			for (auto& storageImage : spvResource.storage_images) {
				StorageImage image;
				image.binding = get_decoration(storageImage.id, spv::Decoration::DecorationBinding);
				image.set = get_decoration(storageImage.id, spv::Decoration::DecorationDescriptorSet);
				strcpy(image.name, storageImage.name.c_str());
				m_vecStorageImages.push_back(image);
				addDescriptorRecord(image.set, image.binding);
			}
			//
			for (auto& pass : spvResource.subpass_inputs) {
				SubpassInput input;
				input.set = get_decoration(pass.id, spv::Decoration::DecorationDescriptorSet);
				input.binding = get_decoration(pass.id, spv::Decoration::DecorationBinding);
				input.inputIndex = get_decoration(pass.id, spv::Decoration::DecorationIndex);
				strcpy(input.name, pass.name.c_str());
				m_vecInputAttachment.push_back(input);
				addDescriptorRecord(input.set, input.binding);
			}
			return true;
		}
	}

}

extern "C" {
	Nix::spvcompiler::IShaderCompiler * GetVkShaderCompiler() {
		if (!compilerInstance) {
			compilerInstance = new Nix::spvcompiler::VkShaderCompiler();
			if (!compilerInstance->initializeEnvironment()) {
				return nullptr;
			}
		}
		Nix::spvcompiler::VkShaderCompiler* compiler = (Nix::spvcompiler::VkShaderCompiler*)compilerInstance;
		compiler->retain();
		return compilerInstance;
	}
}
