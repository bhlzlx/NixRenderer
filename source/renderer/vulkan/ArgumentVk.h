#pragma once
#include <NixRenderer.h>
#include <vector>
#include "VkInc.h"

namespace Nix {
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
		std::vector<std::pair<TextureVk*, SamplerState>>		m_textures;
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
		std::vector<VkDescriptorBufferInfo>						m_vecBufferInfo;
		std::vector<VkDescriptorImageInfo>						m_vecImageInfo; // sampler/image
		//
		std::vector<VkWriteDescriptorSet>						m_vecDescriptorWrites;
		//
		bool													m_needUpdate;
		//
		std::vector<unsigned char>								m_uniformCache;
		uint32_t												m_constantsShaderStages;
	public:
		ArgumentVk();
		~ArgumentVk();

		void bind(VkCommandBuffer _commandBuffer);

		virtual bool getUniformBlock(const char * _name, uint32_t* offset_, const GLSLStructMember** _members, uint32_t* _numMember) override;
		virtual void setUniform(uint32_t _offset, const void* _data, uint32_t _size) override;

		virtual bool getStorageBuffer(const char* _name, uint32_t* id_) override;
		//
		virtual bool getSampler(const char* _name, uint32_t* id_) override; // sampler object
		virtual bool getTexture(const char* _name, uint32_t* id_) override; // sampled image
		virtual bool getStorageImage(const char* _name, uint32_t* id_) override; // storage image
		virtual bool getCombinedImageSampler(const char* _name, uint32_t* id_) override; // combined image sampler
		virtual bool getTexelBuffer(const char * _name, uint32_t* id_) override; // texel buffer
																		   //
		virtual void setStorageBuffer(uint32_t _offset, const void * _data, uint32_t _size) override;
		//
		virtual void setSampler(uint32_t _id, const SamplerState& _sampler) override;
		// 这里注意一下，setTexture/setStorageImage实际上设置流程都一样的，需要的是 VkImage / VkImageView
		virtual void setTexture(uint32_t _id, ITexture* _texture) override;
		virtual void setStorageImage(uint32_t _id, ITexture* _texture) override;
		// 而 setTexelBuffer 需要的是 VkImage / VkBufferView
		virtual void setTexelBuffer(uint32_t _id, ITexture* _texture) override;
		virtual void setCombinedImageSampler(uint32_t _id, const SamplerState& _sampler, ITexture* _texture);
		virtual void setShaderCache(uint32_t _offset, const void* _data, uint32_t _size) override;
		virtual void release() override;
		//
	public:
		void assignUniformChunks();
	};
}