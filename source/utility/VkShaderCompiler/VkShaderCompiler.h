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

	typedef IntegerComposer<uint32_t, uint16_t> DescriptorKey;

	struct Binding {
		ShaderDescriptorType	type = SDT_UniformBlock;
		std::string				name;
	};

	struct StageIOAttribute {
		uint16_t location;
		VertexType type;
		std::string  name;
	};

	struct SubpassInput {
		uint16_t set;
		uint16_t binding;
		uint16_t inputIndex;
	};

	struct PushConstants {
		uint16_t offset;
		uint16_t size;
	};

	struct UniformBlock {
		struct Member {
			uint16_t offset;
			uint16_t size;
			std::string name;
		};
		uint16_t set;
		uint16_t binding;
		uint16_t size;
		std::string name;
	};

	struct ShaderStorageBufferObject {
		uint16_t set;
		uint16_t binding;
		std::string name;
		size_t size;
	};

	struct TexelBufferObject {
		uint16_t set;
		uint16_t binding;
		std::string name;
	};

	struct AtomicCounter {
		uint16_t set;
		uint16_t binding;
		std::string name;s
	};

	struct CombinedImageSampler {
		uint16_t set;
		uint16_t binding;
		std::string name;
	};

	class VkShaderCompiler {
	private:
		std::vector<uint32_t>	m_compiledSPV; // binary spv data
		std::string				m_compileMessage; // compiling error/warnings
		//
		std::vector<uint16_t>	m_vecSetID; // stores the set id
		std::map<uint16_t, std::map<uint16_t, Binding>> m_bindingMap; // set as key, binding as value
		//
		std::map< uint16_t, StageIOAttribute>	m_vecStageInput;
		std::map< uint16_t, StageIOAttribute>	m_vecStageOutput;
		PushConstants							m_pushConstants;
		//
		std::vector<SubpassInput>				m_inputAttachment;
		std::vector<UniformBlock>				m_vecUBO;
		std::vector<CombinedImageSampler>		m_vecSamplers;
		std::vector<ShaderStorageBufferObject>	m_vecSSBO;
		std::vector<TexelBufferObject>			m_vecTBO;
		std::vector<AtomicCounter>				m_atomicCounter;
	private:
		static VkShaderCompiler* Instance;
	public:
		VkShaderCompiler() {
			assert(!Instance);
			Instance = this;
		}
		// 初始化/清理 生产环境
		bool initializeEnvironment();
		void finalizeEnvironment();
		// 将 GLSL 编译成 SPV
		bool compile(Nix::ShaderModuleType _type, const char* _text);
		// 解析SPV
		bool parseSPV(Nix::ShaderModuleType _type, uint32_t*  _spv, size_t _numU32);
		// 获取信息		
		uint16_t getDescriptorSets(const std::function<void(uint16_t _setID)>& _cb) const;
		bool getDescriptor(uint16_t _setID, const std::function<void(uint16_t _binding, ShaderDescriptorType _type, const char * _name)>& _cb) const;
		bool getBlockMemoryLayout( uint16_t _setID, uint16_t _binding, const std::function<void(const char *_name, uint32_t _offset, uint32_t _size)>& _cb ) const;
	};
}

extern "C" {

	NIX_API_DECL bool InitializeShaderCompiler();
	NIX_API_DECL bool CompileGLSL2SPV( Nix::ShaderModuleType _type, const char * _text, const uint32_t** _spvBytes, size_t* _length, const char ** _compileLog);
	NIX_API_DECL void FinalizeShaderCompiler();

	typedef bool(*PFN_INITIALIZE_SHADER_COMPILER)();
	typedef bool(*PFN_COMPILE_GLSL_2_SPV)(Nix::ShaderModuleType _type, const char * _text, const uint32_t** _spvBytes, size_t* _length, const char** _compileLog);
	typedef void(*PFN_FINALIZE_SHADER_COMPILER)();

}