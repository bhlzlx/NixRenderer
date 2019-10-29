#pragma once
#include <NixRenderer.h>
#include <vector>
#include <mutex>
#include <map>
#include <string>
#include <stdint.h>
#include <functional>
#include <cassert>

namespace Nix {
	// 目标
	// 1.生成SPV
	// 2.获取SPV信息
	namespace spvcompiler {
		typedef IntegerComposer<uint32_t, uint16_t> DescriptorKey;
		//
		struct StageIOAttribute {
			uint16_t location;
			VertexType type;
			char name[MaxDescriptorNameLength];
		};

		struct SubpassInput {
			uint16_t set;
			uint16_t binding;
			uint16_t inputIndex;
			char name[MaxDescriptorNameLength];
		};

		struct PushConstants {
			uint16_t offset;
			uint16_t size;
		};

		struct UniformBuffer {
			uint16_t set;
			uint16_t binding;
			uint32_t size;
			char name[MaxDescriptorNameLength];
		};

		struct StorageBuffer {
			uint16_t set;
			uint16_t binding;
			char name[MaxDescriptorNameLength];
			size_t size;
		};

		//struct TexelBufferObject {
		//	uint16_t set;
		//	uint16_t binding;
		//	char name[MaxDescriptorNameLength];
		//};

		//struct AtomicCounter {
		//	uint16_t set;
		//	uint16_t binding;
		//	char name[MaxDescriptorNameLength];
		//};

		struct SeparateSampler {
			uint16_t set;
			uint16_t binding;
			char name[MaxDescriptorNameLength];
		};
		struct SeparateImage {
			uint16_t set;
			uint16_t binding;
			char name[MaxDescriptorNameLength];
		};
		struct StorageImage {
			uint16_t set;
			uint16_t binding;
			char name[MaxDescriptorNameLength];
		};
		struct CombinedImageSampler {
			uint16_t set;
			uint16_t binding;
			char name[MaxDescriptorNameLength];
		};


		class NIX_API_DECL IShaderCompiler {
		private:
		public:
			virtual bool initializeEnvironment() = 0;
			virtual void finalizeEnvironment() = 0;
			// 将 GLSL 编译成 SPV
			virtual bool compile(Nix::ShaderModuleType _type, const char* _text) = 0;
			virtual bool getCompiledSpv(const uint32_t** _spv, size_t* _numU32) const = 0;
			// 解析SPV
			virtual bool parseSpvLayout(Nix::ShaderModuleType _type, const uint32_t* _spv, size_t _numU32) = 0;
			//
			virtual uint16_t getDescriptorSets(const uint16_t** _sets) const = 0;
			virtual uint16_t getBindings(uint16_t _setID, const uint16_t** _bindings) = 0;
			// stage in/out
			virtual uint16_t getStageInput(const StageIOAttribute** _inputs) = 0;
			virtual uint16_t getStageOutput(const StageIOAttribute** _outputs) = 0;
			// buffer descriptor
			virtual uint16_t getUniformBuffers(const UniformBuffer** _blocks) = 0;
			virtual uint16_t getUniformBufferMemebers(uint16_t _set, uint16_t _binding, const GLSLStructMember** _member) = 0;
			virtual uint16_t getShaderStorageBuffers(const StorageBuffer** _ssbo) = 0;
			// image descriptor
			virtual uint16_t getSamplers(const SeparateSampler** _samplers) = 0;
			virtual uint16_t getSampledImages(const SeparateImage** _iamges) = 0;
			virtual uint16_t getStorageImages(const StorageImage** _images) = 0;
			virtual uint16_t getCombinedImageSampler(const CombinedImageSampler** _images) = 0;
			virtual uint16_t getInputAttachment(const SubpassInput** _attachments) = 0;
			// constants
			virtual void getPushConstants(uint16_t* _offset, uint16_t* _size) = 0;
			//
			virtual void retain() = 0;
			virtual void release() = 0;
		};
	}

}

extern "C" {
	typedef Nix::spvcompiler::IShaderCompiler*(*PFN_GETSHADERCOMPILER)();
	NIX_API_DECL Nix::spvcompiler::IShaderCompiler* GetVkShaderCompiler();
}
