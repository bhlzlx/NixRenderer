#include "ArgumentVk.h"
#include "TextureVk.h"
#include "SwapchainVk.h"
#include "BufferVk.h"
#include "QueueVk.h"
#include "ContextVk.h"
#include "DescriptorSetVk.h"
#include "DeferredDeletor.h"

namespace Ks {

	UniformSlot ArgumentVk::getUniformSlot(const char * _name)
	{
		uint32_t setIdx = 0;
		uint32_t uniformIdx;
		uint32_t offset;
		if ( m_descriptorSet->getUniform(_name, uniformIdx, offset))
		{
			UniformSlot slot;
			slot.vkSet = setIdx;
			slot.vkUBOIndex = uniformIdx;
			slot.vkUBOOffset = offset;
			slot.vkUBOSize = 0;
			return slot;
		}
		assert(false);
		UniformSlot slot;
		slot.vkSet = -1;
		slot.vkUBOIndex = -1;
		slot.vkUBOOffset = 0;
		slot.vkUBOSize = 0;
		return slot;
	}

	SamplerSlot ArgumentVk::getSamplerSlot(const char * _name)
	{
		uint32_t setIdx = 0;
		uint32_t binding;
		if (m_descriptorSet->getSampler(_name, binding))
		{
			SamplerSlot slot;
			slot.vkSet = setIdx;
			slot.vkBinding = binding;
			return slot;
		}
		assert(false);
		SamplerSlot slot;
		slot.vkSet = -1;
		slot.vkBinding = -1;
		return slot;
	}

	void ArgumentVk::setUniform(UniformSlot _slot, const void * _data, size_t _size)
	{
		m_descriptorSet->setUniform(_slot.vkUBOIndex, _data, _slot.vkUBOOffset, _size);
	}

	void ArgumentVk::setSampler(SamplerSlot _slot, const SamplerState& _sampler, const ITexture* _texture) {
		VkImage image = ((TextureVk*)_texture)->getImage();
		m_descriptorSet->setSampler(_slot.vkBinding, _sampler, (TextureVk*)_texture);
	}

	void ArgumentVk::bind()
	{
		auto cmd = m_context->getGraphicsQueue()->commandBuffer();
		m_descriptorSet->bind( *cmd, m_context->getSwapchain()->getFlightIndex());
	}

	void ArgumentVk::release()
	{
		auto deletor = GetDeferredDeletor();
		deletor.destroyResource(this);
	}

	ArgumentVk::ArgumentVk( DescriptorSetVk * _set )
	{
		m_descriptorSet = _set;
	}

	ArgumentVk::~ArgumentVk()
	{
		// destroy descriptors
		if( m_descriptorSet )
			m_descriptorSet->release();
		// self delete
		delete this;
	}

}