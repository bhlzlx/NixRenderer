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

	ArgumentVk::ArgumentVk()
		: m_descriptorSetIndex(0)
		, m_activeIndex(0)
		, m_device( VK_NULL_HANDLE )
		, m_material(nullptr)
	{
	}

	ArgumentVk::~ArgumentVk()
	{

	}

	void ArgumentVk::bind()
	{

	}

	bool ArgumentVk::getUniformBlock(const std::string& _name, uint32_t* id_ )
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

	}

	void ArgumentVk::setUniform(uint32_t _index, uint32_t _offset, const void * _data, uint32_t _size)
	{

	}

	void ArgumentVk::release()
	{

	}

	void ArgumentVk::assignUniformChunks()
	{
		const auto& descriptorSetLayout = m_material->getDescriptorSetLayout(m_descriptorSetIndex);
		// get all uniform block size & binding slot);
		std::vector<VkDescriptorBufferInfo> vecBufferInfo;
		std::vector<VkWriteDescriptorSet> vecWrites;

		for (auto& uniformBlockDescriptor : descriptorSetLayout.m_uniformBlockDescriptor) {
			UBOAllocation
		}

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
	}

}