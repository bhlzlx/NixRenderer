#include <NixRenderer.h>
#include <vector>
#include "vkinc.h"

namespace nix {
	class PipelineVk;
	class ContextVk;
	
	// `ArgumentVk` is a wrapper class for VkDescriptorSet

	class NIX_API_DECL ArgumentVk : public IArgument
	{
		friend class VkDeferredDestroyer;
		friend class PipelineVk;
		friend class DescriptorSetPool;
	private:

		std::vector< BufferVk* >	m_uniformBlocks;
		std::vector< std::pair< TextureVk*, SamplerState> >		m_textures;

		ArgumentDescription		m_description;
		uint32_t				m_descriptorSetIndex;
		VkDescriptorSet			m_descriptorSets[2];			//
		uint32_t				m_descriptorSetPools[2];		// pools that holds the descriptor sets
		uint32_t				m_descriptorSetIndex;
		uint32_t				m_activeIndex;
		ContextVk*				m_context;
		MaterialVk*				m_material;
	public:
		ArgumentVk();
		~ArgumentVk();

		virtual void bind() override;
		virtual UniformSlot getUniformSlot(const char * _name) override;
		virtual SamplerSlot getSamplerSlot(const char * _name) override;
		virtual void setSampler(SamplerSlot _slot, const SamplerState& _sampler, const ITexture* _texture) override;
		virtual void setUniform(UniformSlot _slot, const void * _data, size_t _size) override;
		virtual void release() override;
	public:
		void assignUniformChunks();
	};
}