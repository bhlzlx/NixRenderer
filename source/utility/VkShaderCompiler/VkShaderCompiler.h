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

	template< class T1, class T2 >
	class IntegerComposer {
	private:
		union {
			struct {
				T2 a;
				T2 b;
			};
			T1 t;
		};
	public:
		IntegerComposer( T2 _a = 0, T2 _b = 0)
			: a(_a)
			, b(_b) {
		}
		operator T1 () const {
			return t
		}
		operator a() const {
			return a;
		}
		operator b() const {
			return b;
		}
	};
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
		std::string name;
	};

	struct CombinedImageSampler {
		uint16_t set;
		uint16_t binding;
		std::string name;
	};

	class VkShaderCompiler {
	private:
		std::vector<uint32_t>							m_compiledSPV; // binary spv data
		std::string										m_compileMessage; // compiling error/warnings
		//
		std::vector<uint16_t>							m_vecSetID; // stores the set id
		std::map<uint16_t, std::map<uint16_t, Binding>> m_bindingMap; // set as key, binding as value
		std::map < uint32_t, std::vector<UniformBlock::Member>> m_UBOMemberInfo; //
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
		bool getCompiledSpv( const uint32_t*& _spv, size_t& _numU32 ) const;
		// 解析SPV
		bool parseSpvLayout(Nix::ShaderModuleType _type, uint32_t*  _spv, size_t _numU32);
		// 获取信息
		uint16_t getDescriptorSetCount() {
			return (uint16_t)m_vecSetID.size();
		}

		typedef bool(*PFN_ON_GET_STAGE_ATTRIBUTE)(uint16_t _location, const char* _name, uint16_t _type, void* _userData);
		typedef bool(*PFN_ON_GET_STAGE_ATTRIBUTE)(uint16_t _location, const char* _name, uint16_t _type, void* _userData);
		//
		typedef bool(*PFN_ON_GET_DESCRIPTOR_SET)(uint16_t _setID, void* _userData );
		typedef bool(*PFN_ON_GET_DESCRIPTOR_BINDING)(uint16_t _binding, const char * _name, ShaderDescriptorType _type, void* _userData);
		//
		typedef bool(*PFN_ON_GET_UNIFORM_BLOCK)(uint16_t _setID, uint16_t _binding, const char * _name, uint16_t _size, void* _userData);
		typedef bool(*PFN_ON_GET_UNIFORM_BLOCK_LAYOUT)(uint16_t _setID, uint16_t _binding, const char* _name, uint16_t _offset, uint16_t _size, void* _userData);

		uint16_t getDescriptorSets(PFN_ON_GET_DESCRIPTOR_SET _callback, void* _userData) const {
			uint16_t c = 0;
			for (auto setID : m_vecSetID) {
				if (!_callback(setID, _userData)) {
					return c;
				}
				++c;
			}
			return c;
		}
		void getBindings( uint16_t _setID, PFN_ON_GET_DESCRIPTOR_BINDING _callback, void* _userData) {
			auto it = m_bindingMap.find(_setID);
			if (it == m_bindingMap.end()) {
				return;
			}
			for (auto& binding : it->second) {
				_callback(binding.first, binding.second.name.c_str(), binding.second.type, _userData);
			}
		}
		//
		uint16_t getUniformBlockCount() {
			return (uint16_t)m_vecUBO.size();
		}
		void getUniformBlocks( PFN_ON_GET_UNIFORM_BLOCK _callback, void* _userData) {
			for (auto& ubo : m_vecUBO) {
				_callback(ubo.set, ubo.binding, ubo.name.c_str(), ubo.size, _userData);
			}
		}
		bool getUniformBlockLayout(uint16_t _set, uint16_t _binding, PFN_ON_GET_UNIFORM_BLOCK_LAYOUT _cb, void* _userData) {
			DescriptorKey key(_set, _binding);
			auto it = m_UBOMemberInfo.find(key.operator uint32_t());
			if (it == m_UBOMemberInfo.end()) {
				return false;
			}
			for (auto& member : it->second) {
				_cb(_set, _binding, member.name.c_str(), member.offset, member.size, _userData);
			}
			return true;
		}
		void getPushConstants(  );
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