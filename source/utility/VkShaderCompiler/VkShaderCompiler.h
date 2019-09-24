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
		std::string name;
	};

	struct CombinedImageSampler {
		uint16_t set;
		uint16_t binding;
		std::string name;
	};


	class NIX_API_DECL VkShaderCompiler {
	private:
		std::vector<uint32_t>							m_compiledSPV; // binary spv data
		std::string										m_compileMessage; // compiling error/warnings
		//
		std::vector<uint16_t>							m_vecSetID; // stores the set id
		std::map<uint16_t, std::vector<uint16_t>>		m_bindingMap; // set as key, binding as value
		std::map < uint32_t, std::vector<UniformBlock::Member>> 
														m_UBOMemberInfo; //
		//
		std::vector<StageIOAttribute>					m_vecStageInput;
		std::vector<StageIOAttribute>					m_vecStageOutput;
		PushConstants									m_pushConstants;
		//
		std::vector<SubpassInput>						m_inputAttachment;
		std::vector<UniformBlock>						m_vecUBO;
		std::vector<CombinedImageSampler>				m_vecSamplers;
		std::vector<ShaderStorageBufferObject>			m_vecSSBO;
		std::vector<TexelBufferObject>					m_vecTBO;
		std::vector<AtomicCounter>						m_atomicCounter;
	private:
		static VkShaderCompiler* Instance;
		bool											m_initailized;
		//
		void addDescriptorRecord(uint16_t _id, uint16_t _binding);
		void clearResourceInfo();
	public:
		VkShaderCompiler() {
			assert(!Instance);
			Instance = this;
			m_initailized = false;
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

		uint16_t getDescriptorSets( const uint16_t** _sets ) const {
			uint16_t c = m_vecSetID.size();
			if (c) {
				*_sets = m_vecSetID.data();
			}
			return c;
		}
		uint16_t getBindings( uint16_t _setID, const uint16_t** _bindings ) {
			auto it = m_bindingMap.find(_setID);
			if (it == m_bindingMap.end()) {
				return 0;
			}
			uint16_t count = it->second.size();
			*_bindings = it->second.data();
			return count;
		}
		uint16_t getUniformBlocks(const UniformBlock** _blocks) {
			uint16_t count = m_vecUBO.size();
			if (count) {
				*_blocks = m_vecUBO.data();
			}
			return count;
		}
		uint16_t getUniformBlockMembers(uint16_t _set, uint16_t _binding, const UniformBlock::Member** _member ) {
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
			if (m_inputAttachment.size()) {
				*_attachments = m_inputAttachment.data();
			}
			return m_inputAttachment.size();
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
		uint16_t getShaderStorageBuffers(const ShaderStorageBufferObject** _ssbo) {
			if (m_vecSSBO.size()) {
				*_ssbo = m_vecSSBO.data();
			}
			return m_vecSSBO.size();
		}
		uint16_t getSamplers(const CombinedImageSampler** _samplers) {
			if (m_vecSamplers.size()) {
				*_samplers = m_vecSamplers.data();
			}
			return m_vecSamplers.size();
		}
		uint16_t getTexelBuffer(const TexelBufferObject** _tbo) {
			if (m_vecTBO.size()) {
				*_tbo = m_vecTBO.data();
			}
			return m_vecTBO.size();
		}
		uint16_t getAtomicCounter( const AtomicCounter** _counter ) {
			if (m_atomicCounter.size()) {
				*_counter = m_atomicCounter.data();
			}
			return m_atomicCounter.size();
		}
	};

	extern VkShaderCompiler compiler;
}

extern "C" {
	Nix::VkShaderCompiler* GetVkCompiler();
}