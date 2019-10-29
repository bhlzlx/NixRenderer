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

	void ArgumentVk::bind(VkCommandBuffer _commandBuffer, VkPipelineBindPoint _bindPoint)
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
		uint32_t flightIndex = m_context->getSwapchain()->getFlightIndex();
		vkCmdBindDescriptorSets(
			_commandBuffer,
			_bindPoint,
			m_material->getPipelineLayout(),
			m_descriptorSetIndex,
			1,
			&m_descriptorSets[m_activeIndex],
			(uint32_t)m_dynamicalOffsets[flightIndex].size(),
			m_dynamicalOffsets[flightIndex].data()
		);
	}

	void ArgumentVk::transfromImageLayoutIn() {
		for (auto& bindedTex : m_bindedTexture) {
			TextureVk* tex = (TextureVk*)std::get<0>(bindedTex.second);
			VkImageLayout layout = std::get<1>(bindedTex.second);
			m_context->getGraphicsQueue()->tranformImageLayout(tex, layout);
		}
	}

	void ArgumentVk::transfromImageLayoutOut() {
		for (auto& bindedTex : m_bindedTexture) {
			TextureVk* tex = (TextureVk*)std::get<0>(bindedTex.second);
			VkImageLayout layout = std::get<1>(bindedTex.second);
			m_context->getGraphicsQueue()->tranformImageLayout(tex, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}
	}

	bool ArgumentVk::getUniformBlockLayout(const char * _name, const GLSLStructMember ** _members, uint32_t * _numMember)
	{
		const ArgumentLayoutExt& argLayout = m_material->getDescriptorSetLayout(m_descriptorSetIndex);
		const std::vector<GLSLStructMember>* members;
		uint32_t localOffset = 0;
		const UniformBuffer* buffer = argLayout.getUniform(std::string(_name), members);
		if (!buffer) {
			return false;
		}
		*_members = members->data();
		*_numMember = members->size();
		return true;
	}

	bool ArgumentVk::getStorageBufferLocation(const char * _name, uint32_t& id_)
	{
		uint32_t i = 0;
		uint16_t binding = -1;
		const ArgumentLayoutExt& argLayout = m_material->getDescriptorSetLayout(m_descriptorSetIndex);
		for (; i < argLayout.m_vecStorageBuffer.size(); ++i) {
			if (strcmp(argLayout.m_vecStorageBuffer[i].name, _name) == 0) {
				binding = argLayout.m_vecStorageBuffer[i].binding;
				break;
			}
		}
		if (binding != -1) {
			for (i = 0; i < m_vecDescriptorWrites.size(); ++i) {
				if (m_vecDescriptorWrites[i].dstBinding == binding) {
					id_ = i;
					return true;
				}
			}
		}
		return false;
	}

	// 实际上这个 [id] 是 m_vecDescriptorWrites 的下标
	bool ArgumentVk::getSamplerLocation(const char* _name, uint32_t& id_)
	{
		uint32_t i = 0;
		uint16_t binding = -1;
		const ArgumentLayoutExt& argLayout = m_material->getDescriptorSetLayout(m_descriptorSetIndex);
		for (; i < argLayout.m_vecSampler.size(); ++i) {
			if (strcmp(argLayout.m_vecSampler[i].name, _name) == 0) {
				binding = argLayout.m_vecSampler[i].binding;
				break;
			}
		}
		if (binding != -1) {
			for (i = 0; i < m_vecDescriptorWrites.size(); ++i) {
				if (m_vecDescriptorWrites[i].dstBinding == binding) {
					id_ = i;
					return true;
				}
			}
		}
		return false;
	}

	void ArgumentVk::bindSampler(uint32_t _id, const SamplerState& _sampler)
	{
		auto& write = m_vecDescriptorWrites[_id];
		VkDescriptorImageInfo* imageInfo = const_cast<VkDescriptorImageInfo*>(write.pImageInfo);
		imageInfo->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo->imageView = VK_NULL_HANDLE;
		imageInfo->sampler = m_context->getSampler(_sampler);
		m_needUpdate = true;
	}

	bool ArgumentVk::getTextureLocation(const char* _name, uint32_t& id_)
	{
		uint32_t i = 0;
		uint16_t binding = -1;
		const ArgumentLayoutExt& argLayout = m_material->getDescriptorSetLayout(m_descriptorSetIndex);
		for (; i < argLayout.m_vecSampledImage.size(); ++i) {
			if (strcmp(argLayout.m_vecSampledImage[i].name, _name) == 0) {
				binding = argLayout.m_vecSampledImage[i].binding;
				break;
			}
		}
		if (binding != -1) {
			for (i = 0; i < m_vecDescriptorWrites.size(); ++i) {
				if (m_vecDescriptorWrites[i].dstBinding == binding) {
					id_ = i;
					return true;
				}
			}
		}
		return false;
	}

	bool ArgumentVk::getStorageImageLocation(const char* _name, uint32_t& id_)
	{
		uint32_t i = 0;
		uint16_t binding = -1;
		const ArgumentLayoutExt& argLayout = m_material->getDescriptorSetLayout(m_descriptorSetIndex);
		for (; i < argLayout.m_vecStorageImage.size(); ++i) {
			if (strcmp(argLayout.m_vecStorageImage[i].name, _name) == 0) {
				binding = argLayout.m_vecStorageImage[i].binding;
				break;
			}
		}
		if (binding != -1) {
			for (i = 0; i < m_vecDescriptorWrites.size(); ++i) {
				if (m_vecDescriptorWrites[i].dstBinding == binding) {
					id_ = i;
					return true;
				}
			}
		}
		return false;
	}

	bool ArgumentVk::getCombinedImageSamplerLocation(const char* _name, uint32_t& id_)
	{
		uint32_t i = 0;
		uint16_t binding = -1;
		const ArgumentLayoutExt& argLayout = m_material->getDescriptorSetLayout(m_descriptorSetIndex);
		for (; i < argLayout.m_vecCombinedImageSampler.size(); ++i) {
			if (strcmp(argLayout.m_vecCombinedImageSampler[i].name, _name) == 0) {
				binding = argLayout.m_vecCombinedImageSampler[i].binding;
				break;
			}
		}
		if (binding != -1) {
			for (i = 0; i < m_vecDescriptorWrites.size(); ++i) {
				if (m_vecDescriptorWrites[i].dstBinding == binding) {
					id_ = i;
					return true;
				}
			}
		}
		return false;
	}

	bool ArgumentVk::getUniformBlockLocation(const char * _name, uint32_t & _loc)
	{
		uint32_t i = 0;
		uint16_t binding = -1;
		const ArgumentLayoutExt& argLayout = m_material->getDescriptorSetLayout(m_descriptorSetIndex);
		for (; i < argLayout.m_vecUniformBuffer.size(); ++i) {
			if (strcmp(argLayout.m_vecUniformBuffer[i].name, _name) == 0) {
				binding = argLayout.m_vecUniformBuffer[i].binding;
				break;
			}
		}
		if (binding != -1) {
			for (i = 0; i < m_vecDescriptorWrites.size(); ++i) {
				if (m_vecDescriptorWrites[i].dstBinding == binding) {
					_loc = i;
					return true;
				}
			}
		}
		return false;
	}

	bool ArgumentVk::getTexelBufferLocation(const char* _name, uint32_t& id_)
	{
		uint32_t i = 0;
		uint16_t binding = -1;
		const ArgumentLayoutExt& argLayout = m_material->getDescriptorSetLayout(m_descriptorSetIndex);
		for (; i < argLayout.m_vecTexelBuffer.size(); ++i) {
			if (strcmp(argLayout.m_vecTexelBuffer[i].name, _name) == 0) {
				binding = argLayout.m_vecTexelBuffer[i].binding;
				break;
			}
		}
		if (binding != -1) {
			for (i = 0; i < m_vecDescriptorWrites.size(); ++i) {
				if (m_vecDescriptorWrites[i].dstBinding == binding) {
					id_ = i;
					return true;
				}
			}
		}
		return false;
	}

	void ArgumentVk::bindTexture(uint32_t _id, ITexture* _texture)
	{
		auto& write = m_vecDescriptorWrites[_id];
		TextureVk* tex = (TextureVk*)_texture;
		VkDescriptorImageInfo* imageInfo = const_cast<VkDescriptorImageInfo*>(write.pImageInfo);
		imageInfo->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo->imageView = tex->getImageView();
		imageInfo->sampler = VK_NULL_HANDLE;
		m_needUpdate = true;
		//
		m_bindedTexture[_id] = std::tuple<ITexture*, VkImageLayout>(_texture, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	void ArgumentVk::bindStorageImage(uint32_t _id, ITexture* _texture)
	{
		auto& write = m_vecDescriptorWrites[_id];
		TextureVk* tex = (TextureVk*)_texture;
		VkDescriptorImageInfo* imageInfo = const_cast<VkDescriptorImageInfo*>(write.pImageInfo);
		imageInfo->imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		imageInfo->imageView = tex->getImageView();
		imageInfo->sampler = VK_NULL_HANDLE;
		m_needUpdate = true;
		m_bindedTexture[_id] = std::tuple<ITexture*, VkImageLayout>(_texture, VK_IMAGE_LAYOUT_GENERAL);
	}



	void ArgumentVk::bindTexelBuffer(uint32_t _id, IBuffer* _buffer)
	{
		VkWriteDescriptorSet& write = m_vecDescriptorWrites[_id];
		BufferVk* buffer = (BufferVk*)_buffer;
		VkDescriptorBufferInfo* bufferInfo = const_cast<VkDescriptorBufferInfo*>(write.pBufferInfo);
		bufferInfo->offset = buffer->getOffset();
		bufferInfo->buffer = buffer->getHandle();
		bufferInfo->range = buffer->getSize();
		write.pTexelBufferView = VK_NULL_HANDLE;

		const ArgumentLayoutExt& layout = m_material->getDescriptorSetLayout(m_descriptorSetIndex);
		uint32_t index = layout.getDynamicOffsetsIndex(write.dstBinding);
		for (uint32_t i = 0; i < MaxFlightCount; ++i) {
			m_dynamicalOffsets[i][index] = 0;
		}

		assert(false);
		m_needUpdate = true;
	}

	void ArgumentVk::bindCombinedImageSampler(uint32_t _id, const SamplerState& _sampler, ITexture* _texture)
	{
		auto& write = m_vecDescriptorWrites[_id];
		TextureVk* tex = (TextureVk*)_texture;
		VkDescriptorImageInfo* imageInfo = const_cast<VkDescriptorImageInfo*>(write.pImageInfo);
		imageInfo->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo->imageView = tex->getImageView();
		imageInfo->sampler = m_context->getSampler(_sampler);
		m_needUpdate = true;
		m_bindedTexture[_id] = std::tuple<ITexture*, VkImageLayout>(_texture, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	void ArgumentVk::bindStorageBuffer(uint32_t _id, IBuffer * _buffer)
	{
		auto& write = m_vecDescriptorWrites[_id];
		BufferVk* buf = (BufferVk*)_buffer;
		VkDescriptorBufferInfo* bufferInfo = const_cast<VkDescriptorBufferInfo*>(write.pBufferInfo);
		bufferInfo->buffer = buf->getHandle();
		bufferInfo->offset = buf->getOffset();
		bufferInfo->range = buf->getSize();
		//
		const ArgumentLayoutExt& layout = m_material->getDescriptorSetLayout(m_descriptorSetIndex);
		uint32_t index = layout.getDynamicOffsetsIndex(write.dstBinding);
		for (uint32_t i = 0; i < MaxFlightCount; ++i) {
			m_dynamicalOffsets[i][index] = 0;
		}
		m_needUpdate = true;
	}

	void ArgumentVk::bindUniformBuffer(uint32_t _id, IBuffer * _buffer)
	{
		auto& write = m_vecDescriptorWrites[_id];
		BufferVk* buf = (BufferVk*)_buffer;
		VkDescriptorBufferInfo* bufferInfo = const_cast<VkDescriptorBufferInfo*>(write.pBufferInfo);
		bufferInfo->buffer = buf->getHandle();
		bufferInfo->offset = buf->getOffset();
		bufferInfo->range = buf->getSize() / MaxFlightCount;
		//
		const ArgumentLayoutExt& layout = m_material->getDescriptorSetLayout(m_descriptorSetIndex);
		uint32_t index = layout.getDynamicOffsetsIndex(write.dstBinding);
		for (uint32_t i = 0; i < MaxFlightCount; ++i) {
			m_dynamicalOffsets[i][index] = (buf->getSize() / MaxFlightCount) * i;
		}
		m_needUpdate = true;
	}

	void ArgumentVk::updateUniformBuffer(IBuffer* _buffer, const void* _data, uint32_t _offset, uint32_t _length)
	{
		uint32_t flightIndex = m_context->getSwapchain()->getFlightIndex();
		_buffer->updateData(_data, _length, _offset + _buffer->getSize() / MaxFlightCount * flightIndex);
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

}
