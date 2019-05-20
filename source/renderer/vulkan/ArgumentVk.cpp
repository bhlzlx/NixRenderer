#include "ArgumentVk.h"
#include "TextureVk.h"
#include "SwapchainVk.h"
#include "BufferVk.h"
#include "QueueVk.h"
#include "ContextVk.h"
#include "MaterialVk.h"
#include "DescriptorSetVk.h"
#include "DeferredDeletor.h"

namespace nix {

	IArgument* MaterialVk::createArgument(uint32_t _index) {
		ArgumentVk* argument = m_context->getDescriptorSetPool().allocateArgument(this, _index);
		return argument;
	}

	void assignUniformChunks() {
		auto context = (ContextVk*)GetContextVulkan();
		// get all uniform block size & binding slot
		std::vector<UniformBindingPoint> bindingPoints;
		m_pipeline->getUniformBindingPoints(m_activeDescriptor.id, bindingPoints);
		//
		auto& uniformAllocator = context->getUBOAllocator();
		std::vector<VkDescriptorBufferInfo> vecBufferInfo;
		vecBufferInfo.reserve(32);
		std::vector<VkWriteDescriptorSet> vecWrites;

		//UniformChunkWriteData writeData;
		for (auto& bindingPoint : bindingPoints)
		{
			// allocation size is the `MaxFlightCount` times size of the binding point size
			// see the `UniformStaticAllocator` implementation
			UBOAllocation allocation = uniformAllocator.alloc(bindingPoint.size);
			VkDescriptorBufferInfo bufferInfo; {
				bufferInfo.buffer = (const VkBuffer&)*allocation.buffer;
				bufferInfo.offset = 0;// allocation.offset;
				bufferInfo.range = allocation.capacity;
			}
			vecBufferInfo.push_back(bufferInfo);
			VkWriteDescriptorSet writeDescriptorSet = {}; {
				writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDescriptorSet.pNext = nullptr;
				writeDescriptorSet.dstSet = m_activeDescriptor.set;
				writeDescriptorSet.dstArrayElement = 0;
				writeDescriptorSet.descriptorCount = 1;
				writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
				writeDescriptorSet.pBufferInfo = &vecBufferInfo.back();
				writeDescriptorSet.pImageInfo = nullptr;
				writeDescriptorSet.pTexelBufferView = nullptr;
				writeDescriptorSet.dstBinding = bindingPoint.binding;
			}
			vecWrites.push_back(writeDescriptorSet);
			// we should also store the `MaxFlightCount` frame's dynamic offsets
			for (uint32_t i = 0; i < MaxFlightCount; ++i)
			{
				auto& vecOffsets = m_dynamicOffsets[i];
				vecOffsets.push_back(static_cast<uint32_t>(allocation.capacity / MaxFlightCount * i) + static_cast<uint32_t>(allocation.offset));
			}
			//
			m_vecUBOChunks.resize(m_vecUBOChunks.size() + 1);
			m_vecUBOChunks.back().binding = bindingPoint.binding;
			m_vecUBOChunks.back().uniform = allocation;
		}

		vkUpdateDescriptorSets(context->getDevice(), static_cast<uint32_t>(vecWrites.size()), vecWrites.data(), 0, nullptr);

		return true;
	}

	UniformSlot ArgumentVk::getUniformSlot(const char * _name)
	{
		uint32_t setIdx = 0;
		uint32_t uniformIdx;
		uint32_t offset;
		if ( m_descriptorSet->getUniform(_name, uniformIdx, offset) )
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

	ArgumentVk::ArgumentVk()
	{
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