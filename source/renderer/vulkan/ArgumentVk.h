#include <NixRenderer.h>
#include <vector>
#include "vkinc.h"

namespace Ks {
	class DescriptorSetVk;
	class PipelineVk;
	class ContextVk;

	class KS_API_DECL ArgumentVk : public IArgument
	{
		friend class VkDeferredDestroyer;
		friend class PipelineVk;
	public:
	private:
		DescriptorSetVk* m_descriptorSet;
		ContextVk* m_context;
		PipelineVk* m_pipeline;
	public:
		ArgumentVk( DescriptorSetVk* _set );
		~ArgumentVk();

		virtual void bind() override;
		virtual UniformSlot getUniformSlot(const char * _name) override;
		virtual SamplerSlot getSamplerSlot(const char * _name) override;
		virtual void setSampler(SamplerSlot _slot, const SamplerState& _sampler, const ITexture* _texture) override;
		virtual void setUniform(UniformSlot _slot, const void * _data, size_t _size) override;
		virtual void release() override;
	};
}