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

namespace Nix {

	IArgument* MaterialVk::createArgument(uint32_t _index) {
		ArgumentAllocator& allocator = m_context->getDescriptorSetPool();
		ArgumentVk* argument = allocator.allocateArgument(this, _index);
		argument->assignUniformChunks();
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
			m_activeIndex = m_activeIndex % MaxFlightCount;
			for (auto& write : m_vecDescriptorWrites) {
				write.dstSet = m_descriptorSets[m_activeIndex];
			}
			vkUpdateDescriptorSets(m_context->getDevice(), static_cast<uint32_t>(m_vecDescriptorWrites.size()), m_vecDescriptorWrites.data(), 0, nullptr);
			m_needUpdate = false;
		}
		vkCmdBindDescriptorSets(
			_commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			m_material->getPipelineLayout(),
			m_descriptorSetIndex,
			1,
			&m_descriptorSets[m_activeIndex],
			(uint32_t)m_dynamicalOffsets[m_activeIndex].size(),
			m_dynamicalOffsets[m_activeIndex].data()
		);
	}

	bool ArgumentVk::getUniformBlock(const char * _name, uint32_t* id_, uint32_t* offset_, const GLSLStructMember** _members, uint32_t* _numMember)
	{
		const ArgumentLayoutExt& argLayout = m_material->getDescriptorSetLayout(m_descriptorSetIndex);
		const std::vector<GLSLStructMember>* members;
		uint32_t localOffset = 0;
		const UniformBuffer* buffer = argLayout.getUniform(std::string(_name), members, localOffset);
		if (!buffer) {
			return false;
		}
		*offset_ = localOffset;
		return true;
	}

	bool ArgumentVk::getStorageBuffer(const char * _name, uint32_t * id_)
	{
		const ArgumentLayoutExt& argLayout = m_material->getDescriptorSetLayout(m_descriptorSetIndex);
		auto storageBuffer = argLayout.getStorageBuffer(std::string(_name));
		m_ssbos
	}

	bool ArgumentVk::getSampler(const char* _name, uint32_t* id_)
	{
		uint32_t i = 0;
		const ArgumentLayoutExt& argLayout = m_material->getDescriptorSetLayout(m_descriptorSetIndex);
		for (; i < argLayout.m_vecSampler.size(); ++i) {
			if ( strcmp(argLayout.m_vecSampler[i].name, _name)) {
				*id_ = i;
				return true;
			}
		}
		return false;
	}

	void ArgumentVk::setSampler(uint32_t _index, const SamplerState& _sampler, const ITexture* _texture)
	{
		auto descriptor = m_material->getDescriptorSetLayout(m_descriptorSetIndex).m_vecSampler[_index];
		for (auto& write : m_vecDescriptorWrites) {
			if (write.dstBinding == descriptor.binding) {
				VkDescriptorImageInfo* imageInfo = const_cast<VkDescriptorImageInfo*>(write.pImageInfo);
				imageInfo->imageView = ((TextureVk*)_texture)->getImageView();
				imageInfo->sampler = m_context->getSampler(_sampler);
				imageInfo->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				m_needUpdate = true;
				break;
			}
		}
	}

	void ArgumentVk::setUniform( uint32_t _offset, const void * _data, uint32_t _size) {
		unsigned char* localUniform = m_localUnifrom.data();
		memcpy( localUniform + _offset, _data, _size );
	}

	void ArgumentVk::setShaderCache(uint32_t _offset, const void* _data, uint32_t _size)
	{
#ifndef NDEBUG
		DriverVk* driver = (DriverVk*)m_context->getDriver();
		assert(_size + _offset <= driver->getPhysicalDeviceProperties().limits.maxPushConstantsSize);
#endif
		auto cmd = m_context->getGraphicsQueue()->commandBuffer();
		vkCmdPushConstants(cmd->operator const VkCommandBuffer &(), m_material->getPipelineLayout(), m_material->getConstantsStageFlags(), _offset, _size, _data);
	}

	void ArgumentVk::release()
	{
		m_material->destroyArgument(this);
	}

	void ArgumentVk::assignUniformChunks()
	{
		const auto& descriptorSetLayout = m_material->getDescriptorSetLayout(m_descriptorSetIndex);
		// get all uniform block size & binding slot);
		ContextVk* context = m_material->getContext();
		auto& dynamicalBindings = m_material->getDescriptorSetLayout(m_descriptorSetIndex).m_dynamicalBindings;
		//
		for (auto& dynamcalOffset : m_dynamicalOffsets) {
			dynamcalOffset.resize(dynamicalBindings.size());
		}

		for (auto& uniformBlockDescriptor : descriptorSetLayout.m_uniformBlockDescriptor) {
			IBufferAllocator* allocator = context->uniformAllocator();
			BufferAllocation allocation = allocator->allocate(uniformBlockDescriptor.dataSize);
			m_uniformBlocks.push_back(allocation);
			//
			VkDescriptorBufferInfo * bufferInfo = nullptr;
			for (uint32_t i = 0; i < descriptorSetLayout.m_dynamicalBindings.size(); ++i) {
				if (descriptorSetLayout.m_dynamicalBindings[i] == uniformBlockDescriptor.binding) {
					bufferInfo = &m_vecDescriptorBufferInfo[i];
					break;
				}
			}
			assert(bufferInfo);
			bufferInfo->buffer = (VkBuffer)allocation.buffer;
			bufferInfo->offset = allocation.offset;
			bufferInfo->range = allocation.size / MaxFlightCount;

			VkWriteDescriptorSet writeDescriptorSet = {}; {
				writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDescriptorSet.pNext = nullptr;
				writeDescriptorSet.dstSet = m_descriptorSets[m_activeIndex];
				writeDescriptorSet.dstArrayElement = 0;
				writeDescriptorSet.descriptorCount = 1;
				writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
				writeDescriptorSet.pBufferInfo = bufferInfo;
				writeDescriptorSet.pImageInfo = nullptr;
				writeDescriptorSet.pTexelBufferView = nullptr;
				writeDescriptorSet.dstBinding = uniformBlockDescriptor.binding;
			}
			// update dynamical offset
			auto iter = std::find( dynamicalBindings.begin(), dynamicalBindings.end(), uniformBlockDescriptor.binding);
			assert(iter != dynamicalBindings.end());
			size_t idx = iter - dynamicalBindings.begin();
			for (uint32_t flightIndex = 0; flightIndex < MaxFlightCount; ++flightIndex) {
				m_dynamicalOffsets[flightIndex][idx] = (uint32_t)allocation.size / MaxFlightCount * flightIndex + (uint32_t)allocation.offset;
			}
		}
	}

}