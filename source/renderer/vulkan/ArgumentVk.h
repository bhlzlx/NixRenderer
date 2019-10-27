#pragma once
#include <NixRenderer.h>
#include <vector>
#include <map>
#include "VkInc.h"

namespace Nix {
	class PipelineVk;
	class ContextVk;
	class TextureVk;
	class BufferVk;
	class MaterialVk;

	// `ArgumentVk` is a wrapper class for VkDescriptorSet

	class NIX_API_DECL ArgumentVk : public IArgument
	{
		friend class VkDeferredDestroyer;
		friend class PipelineVk;
		friend class ArgumentAllocator;
	private:
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
		std::vector<uint32_t>									m_vecDynamicOffsetIndices;
		// uniform buffers use the dynamic offsets
		// storage buffer / texel buffer use the static offsets
		std::vector<uint32_t>									m_dynamicalOffsets[MaxFlightCount];
		//
		bool													m_needUpdate;
		//
		uint32_t												m_constantsShaderStages;
		std::map<uint32_t, std::tuple<ITexture*,VkImageLayout>>	m_bindedTexture;
	public:
		ArgumentVk();
		~ArgumentVk();

		void bind(VkCommandBuffer _commandBuffer, VkPipelineBindPoint _bindPoint);
		void transfromImageLayoutIn();
		void transfromImageLayoutOut();

		virtual bool getUniformBlockLayout(const char * _name, const GLSLStructMember** _members, uint32_t* _numMember) override;
		//
		virtual bool getSamplerLocation(const char* _name, uint32_t& _loc) override; // sampler object
		virtual bool getTextureLocation(const char* _name, uint32_t& _loc) override; // sampled image
		virtual bool getStorageImageLocation(const char* _name, uint32_t& _loc) override; // storage image
		virtual bool getCombinedImageSamplerLocation(const char* _name, uint32_t& _loc) override; // combined image sampler
		virtual bool getUniformBlockLocation(const char* _name, uint32_t& _loc) override;
		virtual bool getStorageBufferLocation(const char* _name, uint32_t& _loc) override;
		virtual bool getTexelBufferLocation(const char * _name, uint32_t& _loc) override; // texel buffer

		// all possible functions that will update the descriptor set
		virtual void bindSampler(uint32_t _id, const SamplerState& _sampler) override;
		virtual void bindTexture(uint32_t _id, ITexture* _texture) override;
		virtual void bindStorageImage(uint32_t _id, ITexture* _texture) override;
		virtual void bindCombinedImageSampler(uint32_t _id, const SamplerState& _sampler, ITexture* _texture);
		virtual void bindStorageBuffer(uint32_t _id, IBuffer* _buffer) override;
		virtual void bindUniformBuffer(uint32_t _id, IBuffer* _buffer) override;
		virtual void bindTexelBuffer(uint32_t _id, IBuffer* _buffer) override;
		//
		virtual void updateUniformBuffer(IBuffer* _buffer, const void* _data, uint32_t _offset, uint32_t _length) override;
		virtual void setShaderCache(uint32_t _offset, const void* _data, uint32_t _size) override;

		virtual void release() override;
	};
}