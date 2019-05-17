#pragma once
#include <NixRenderer.h>
#include "vkinc.h"
#include <vector>
#include <list>
#include <map>
#include <array>

namespace Ks {

	class PipelineVk;
	class TextureVk;
	class BufferVk;

	class KS_API_DECL DescriptorSetVk
	{
		friend class DescriptorSetPool;
	private:
		PipelineVk* m_pipeline;
		union {
			DescriptorSet m_descriptorSet[2];
			struct {
				DescriptorSet m_activeDescriptor;
				DescriptorSet m_backupDescriptor;
			};
		};
		struct STPair /* Sampler Texture Pair */ {
			SamplerState sampler;
			TextureVk* texture;
		};

		std::array< std::vector<uint32_t>, MaxFlightCount > m_dynamicOffsets;
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
			return m_activeDescriptor.set;
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

	class KS_API_DECL MinimumDescriptorSetPool {
	private:
		VkDescriptorPool m_pool;
		uint8_t m_state; // 0 : unavailable 
	public:
		MinimumDescriptorSetPool() : m_pool(VK_NULL_HANDLE), m_state(0) {
		}

		bool available() const {
			return m_state != 0;
		}
		void initialize();
		bool allocate(VkDescriptorSetLayout* _layouts, uint32_t _layoutCount, VkDescriptorSet* descriptorSets_);
		void free(VkDescriptorSet* _descriptorSets, uint32_t _setCount);
	};

	class KS_API_DECL DescriptorSetPool
	{
	private:
		std::vector< MinimumDescriptorSetPool > m_vecMiniPools;
	public:
		DescriptorSetPool() {
		}
		std::vector< DescriptorSet > allocOneBatch(PipelineVk* _pipeline );
		DescriptorSet allocOne(PipelineVk* _pipeline, uint32_t _setID);
		//
		//std::vector<DescriptorSetVk*> alloc(PipelineVk* _pipeline);
		DescriptorSetVk* alloc(PipelineVk* _pipeline, int _setId);
		void free(const std::vector<DescriptorSetVk*>& _set);
	};
}