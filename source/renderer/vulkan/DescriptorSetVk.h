#pragma once
#include <NixRenderer.h>
#include "vkinc.h"
#include <vector>
#include <list>
#include <map>
#include <array>

namespace nix {
	class MaterialVk;
	class TextureVk;
	class BufferVk;

	// assume that, we only allocate one `uniform chunk` per `descriptor set`
	class NIX_API_DECL DescriptorSetVk
	{
		friend class DescriptorSetPool;
	private:
		uint32_t		m_descriptorSetID; // descriptor set id declared in the shader
		size_t			m_descriptorSetPoolIndex[2]; // descriptor set chunk index ( in the descriptor set pool object )
		VkDescriptorSet m_descriptorSets[2]; // descriptor set, one is in use and one is for backup
		uint32_t		m_activeDescriptorSetIndex; // the index of the one in use
		//
		std::array< std::array<uint32_t, MaxArgumentCount>, MaxFlightCount > m_dynamicOffsets;
		//
		struct UniformChunkWriteData {
			uint32_t binding;
			UBOAllocation uniform;
		};
		struct SamplerWriteData {
			uint32_t binding;
			SamplerState samplerState;
			TextureVk* texture;
		};
		std::vector< UniformChunkWriteData > m_vecUBOChunks;
		std::vector< SamplerWriteData > m_vecSamplerData;
		//std::vector< SamplerWriteData > m_vecUpdates;
		bool m_needUpdate;
	public:
		// `getUniform`'s return value is the index of the 'm_uniform'
		bool getUniform(const char * _name, uint32_t& index_, uint32_t& offset_);
		void setUniform(size_t _index, const void * _data, size_t _offset, size_t _size);
		template< class T >
		void setUniform(size_t _index, const T& _obj) {
			setUniform(_index, &_obj, sizeof(_obj));
		}
		// `getSampler`'s return value is the `binding slot` of this `descriptor set`
		bool getSampler(const char * _name, uint32_t& binding_);
		void setSampler(size_t _binding, const SamplerState& _samplerState, TextureVk* _texture);
		void performUpdates();
		// initialize the uniform object for the descriptor set
		bool assignUniformObjects();
		//
		void bind(VkCommandBuffer _cmdbuff, uint32_t _flightIndex);
		//
		const VkDescriptorSet& descriptorSet() const {
			return m_descriptorSets[m_activeDescriptorSetIndex];
		}
	private:
		DescriptorSetVk() {
		}
	public:
		~DescriptorSetVk();
		//
		void release();
	};
	// Descriptor set

	const static int ChunkSamplerCount = 1024;
	const static int ChunkUniformBlockCount = 256;

	class NIX_API_DECL DescriptorSetPoolChunk {
	private:
		VkDescriptorPool	m_pool;
		uint8_t				m_state; // 0 : unavailable 
	public:
		DescriptorSetPoolChunk() : m_pool(VK_NULL_HANDLE), m_state(0) {
		}
		bool available() const {
			return m_state != 0;
		}
		void initialize();
		VkDescriptorSet allocate( VkDevice _device, VkDescriptorSetLayout _descSetLayout );
		void free( VkDevice _device, VkDescriptorSet _descSet );
	};

	class NIX_API_DECL DescriptorSetPool
	{
	private:
		std::vector< DescriptorSetPoolChunk > m_vecMiniPools;
	public:
		DescriptorSetPool() {
		}
		//
		DescriptorSetVk* allocate(MaterialVk* _material, uint32_t _index);
		void free( DescriptorSetVk* _descriptorSet );
	};
}