#pragma once
#include <NixRenderer.h>
#include <vector>
#include "UniformVk.h"
#include "vkinc.h"

namespace nix {
	class PipelineVk;
	class ContextVk;
	class TextureVk;
	class MaterialVk;
	
	// `ArgumentVk` is a wrapper class for VkDescriptorSet

	class NIX_API_DECL ArgumentVk : public IArgument
	{
		friend class VkDeferredDestroyer;
		friend class PipelineVk;
		friend class ArgumentAllocator;
	private:
		std::vector< UniformAllocation >						m_uniformBlocks;
		std::vector< std::pair< TextureVk*, SamplerState> >		m_textures;
		std::vector< BufferVk* >								m_ssbos;
		std::vector< uint32_t >									m_dynamicalOffsets[MaxFlightCount];
		//
		uint32_t												m_descriptorSetIndex;
		VkDescriptorSet											m_descriptorSets[MaxFlightCount];			//
		uint32_t												m_descriptorSetPools[MaxFlightCount];		// pools that holds the descriptor sets
		uint32_t												m_activeIndex;
		//VkDevice												m_device;
		ContextVk*												m_context;
		MaterialVk*												m_material;
		//
		std::vector<VkDescriptorBufferInfo>						m_vecDescriptorBufferInfo; // UBO, SSBO, TBO
		std::vector<VkDescriptorImageInfo>						m_vecDescriptorImageInfo; // sampler/image
		std::vector<VkWriteDescriptorSet>						m_vecDescriptorWrites;
		bool													m_needUpdate;
	public:
		ArgumentVk();
		~ArgumentVk();

		virtual void bind() override;
		virtual bool getUniformBlock(const std::string& _name, uint32_t* id_ ) override;
		virtual bool getUniformMemberOffset( uint32_t _uniform, const std::string& _name, uint32_t* offset_) override;
		virtual bool getSampler(const std::string& _name, uint32_t* id_) override;
		//
		virtual void setSampler(uint32_t _index, const SamplerState& _sampler, const ITexture* _texture) override;
		virtual void setUniform(uint32_t _index, uint32_t _offset, const void * _data, uint32_t _size) override;
		virtual void release() override;
	public:
		void assignUniformChunks();
	};
}