#include <NixRenderer.h>
#include <vector>
#include "UniformVk.h"
#include "vkinc.h"

namespace nix {
	class PipelineVk;
	class ContextVk;
	class TextureVk;
	
	// `ArgumentVk` is a wrapper class for VkDescriptorSet

	class NIX_API_DECL ArgumentVk : public IArgument
	{
		friend class VkDeferredDestroyer;
		friend class PipelineVk;
		friend class DescriptorSetPool;
	private:
		std::vector< UBOAllocation >							m_uniformBlocks;
		std::vector< std::pair< TextureVk*, SamplerState> >		m_textures;
		std::vector< BufferVk* >								m_ssbos;
		//
		uint32_t												m_descriptorSetIndex;
		VkDescriptorSet											m_descriptorSets[2];			//
		uint32_t												m_descriptorSetPools[2];		// pools that holds the descriptor sets
		uint32_t												m_activeIndex;
		ContextVk*												m_context;
		MaterialVk*												m_material;
	public:
		ArgumentVk();
		~ArgumentVk();

		virtual void bind() override;
		virtual uint32_t getUniformBlock(const std::string& _name) override;
		virtual uint32_t getUniformMemberOffset(const std::string& _name) override;
		virtual uint32_t getSampler(const std::string& _name) override;
		//
		virtual void setSampler(uint32_t _index, const SamplerState& _sampler, const ITexture* _texture) override;
		virtual void setUniform(uint32_t _index, uint32_t _offset, const void * _data, uint32_t _size) override;
		virtual void release() override;
	public:
		void assignUniformChunks();
	};
}