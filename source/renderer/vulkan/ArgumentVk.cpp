#include "ArgumentVk.h"
#include "TextureVk.h"
#include "SwapchainVk.h"
#include "BufferVk.h"
#include "QueueVk.h"
#include "ContextVk.h"
#include "MaterialVk.h"
#include "DescriptorSetVk.h"
#include "DeferredDeletor.h"
#include "DriverVk.h"

namespace nix {

	VkSampler GetSampler(const SamplerState& _state);

	IArgument* MaterialVk::createArgument(uint32_t _index) {
		ArgumentVk* argument = m_context->getDescriptorSetPool().allocateArgument(this, _index);
		return argument;
	}

	ArgumentVk::ArgumentVk()
		: m_descriptorSetIndex(0)
		, m_activeIndex(0)
		, m_context(nullptr)
		, m_material(nullptr)
		, m_needUpdate(true)
	{
	}

	ArgumentVk::~ArgumentVk()
	{
		// do nothing
	}

	void ArgumentVk::bind( VkCommandBuffer _commandBuffer )
	{
		if (m_needUpdate) {
			++m_activeIndex;
			for (auto& write : m_vecDescriptorWrites) {
				write.dstSet = m_descriptorSets[m_activeIndex];
			}
			vkUpdateDescriptorSets(m_context->getDevice(), static_cast<uint32_t>(m_vecDescriptorWrites.size()), m_vecDescriptorWrites.data(), 0, nullptr);
		}
		vkCmdBindDescriptorSets(
			_commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			m_material->getPipelineLayout(),
			0,
			1,
			&m_descriptorSets[m_activeIndex],
			m_dynamicalOffsets[m_activeIndex].size(),
			m_dynamicalOffsets[m_activeIndex].data()
		);
	}

	bool ArgumentVk::getUniformBlock(const std::string& _name, uint32_t* id_)
	{
		uint32_t i = 0;
		const ArgumentLayout& argLayout = m_material->getDescriptorSetLayout(m_descriptorSetIndex);
		for (; i < argLayout.m_uniformBlockDescriptor.size(); ++i) {
			if (argLayout.m_uniformBlockDescriptor[i].name == _name) {
				*id_ = i;
				return true;
			}
		}
		return false;
	}

	bool ArgumentVk::getUniformMemberOffset(uint32_t _uniform, const std::string& _name, uint32_t* offset_)
	{
		uint32_t i = 0;
		const ArgumentLayout& argLayout = m_material->getDescriptorSetLayout(m_descriptorSetIndex);
		auto binding = argLayout.m_uniformBlockDescriptor[_uniform].binding;
		for (; i < argLayout.m_uniformMembers.size(); ++i) {
			if (argLayout.m_uniformMembers[i].name == _name) {
				*offset_ = argLayout.m_uniformMembers[i].offset;
				return true;
			}
		}
		return false;
	}

	bool ArgumentVk::getSampler(const std::string& _name, uint32_t* id_)
	{
		uint32_t i = 0;
		const ArgumentLayout& argLayout = m_material->getDescriptorSetLayout(m_descriptorSetIndex);
		for (; i < argLayout.m_samplerImageDescriptor.size(); ++i) {
			if (argLayout.m_samplerImageDescriptor[i].name == _name) {
				*id_ = i;
				return true;
			}
		}
		return false;
	}

	void ArgumentVk::setSampler(uint32_t _index, const SamplerState& _sampler, const ITexture* _texture)
	{
		auto descriptor = m_material->getDescriptorSetLayout(m_descriptorSetIndex).m_samplerImageDescriptor[_index];
		for (auto& write : m_vecDescriptorWrites) {
			if (write.dstBinding == descriptor.binding) {
				VkDescriptorImageInfo* imageInfo = const_cast<VkDescriptorImageInfo*>(write.pImageInfo);
				imageInfo->imageView = ((TextureVk*)_texture)->getImageView();
				imageInfo->sampler = GetSampler(_sampler);
				imageInfo->imageLayout = ((TextureVk*)_texture)->getImageLayout();
				m_needUpdate = true;
				break;
			}
		}
	}

	void ArgumentVk::setUniform(uint32_t _index, uint32_t _offset, const void * _data, uint32_t _size)
	{
		auto flightIndex = m_context->getSwapchain()->getFlightIndex();
		auto& uniformBlock = m_uniformBlocks[_index];
		memcpy(uniformBlock.raw + flightIndex*uniformBlock.unitSize + _offset, _data, _size);
	}

	void ArgumentVk::setShaderCache(uint32_t _offset, const void* _data, uint32_t _size)
	{
#ifndef NDEBUG
		DriverVk* driver = (DriverVk*)m_context->getDriver();
		assert(_size + _offset <= driver->getPhysicalDeviceProperties().limits.maxPushConstantsSize);
#endif
		auto cmd = m_context->getGraphicsQueue()->commandBuffer();
		vkCmdPushConstants(cmd->operator const VkCommandBuffer &(), m_material->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, _offset, _size, _data);
	}

	void ArgumentVk::release()
	{
		m_material->destroyArgument(this);
	}

	void ArgumentVk::assignUniformChunks()
	{
		const auto& descriptorSetLayout = m_material->getDescriptorSetLayout(m_descriptorSetIndex);
		// get all uniform block size & binding slot);
		std::vector<VkDescriptorBufferInfo> vecBufferInfo;
		std::vector<VkWriteDescriptorSet> vecWrites;
		ContextVk* context = m_material->getContext();
		auto& dynamicalBindings = m_material->getDescriptorSetLayout(m_descriptorSetIndex).m_dynamicalBindings;
		//
		for (auto& dynamcalOffset : m_dynamicalOffsets) {
			dynamcalOffset.resize(dynamicalBindings.size());
		}

		for (auto& uniformBlockDescriptor : descriptorSetLayout.m_uniformBlockDescriptor) {
			UniformAllocation allocation;
			bool rst = context->getUniformAllocator().allocate(uniformBlockDescriptor.dataSize, allocation);
			m_uniformBlocks.push_back(allocation);
			//
			VkDescriptorBufferInfo bufferInfo; {
				bufferInfo.buffer = allocation.buffer;
				bufferInfo.offset = allocation.offset;
				bufferInfo.range = allocation.unitSize * MaxFlightCount;
			}
			vecBufferInfo.push_back(bufferInfo);
			VkWriteDescriptorSet writeDescriptorSet = {}; {
				writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDescriptorSet.pNext = nullptr;
				writeDescriptorSet.dstSet = m_descriptorSets[m_activeIndex];
				writeDescriptorSet.dstArrayElement = 0;
				writeDescriptorSet.descriptorCount = 1;
				writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
				writeDescriptorSet.pBufferInfo = &vecBufferInfo.back();
				writeDescriptorSet.pImageInfo = nullptr;
				writeDescriptorSet.pTexelBufferView = nullptr;
				writeDescriptorSet.dstBinding = uniformBlockDescriptor.binding;
			}
			vecWrites.push_back(writeDescriptorSet);
			// update dynamical offset
			auto iter = std::find( dynamicalBindings.begin(), dynamicalBindings.end(), uniformBlockDescriptor.binding);
			assert(iter != dynamicalBindings.end());
			size_t idx = iter - dynamicalBindings.begin();
			for (uint32_t flightIndex = 0; flightIndex < MaxFlightCount; ++flightIndex) {
				m_dynamicalOffsets[flightIndex][idx] = allocation.unitSize * flightIndex;
			}
		}
	}

}