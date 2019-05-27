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
	{

	}

	ArgumentVk::~ArgumentVk()
	{

	}

	void ArgumentVk::bind()
	{

	}

	uint32_t ArgumentVk::getUniformBlock(const std::string& _name)
	{

	}

	uint32_t ArgumentVk::getUniformMemberOffset(const std::string& _name)
	{

	}

	uint32_t ArgumentVk::getSampler(const std::string& _name)
	{

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
		const auto& descriptorSetLayout = m_material->m_descriptorSetLayouts[ m_descriptorSetIndex ];
		//descriptorSetLayout.
		// 		auto context = (ContextVk*)GetContextVulkan();
		// 		// get all uniform block size & binding slot
		// 		std::vector<UniformBindingPoint> bindingPoints;
		// 		m_pipeline->getUniformBindingPoints(m_activeDescriptor.id, bindingPoints);
		// 		//
		// 		auto& uniformAllocator = context->getUBOAllocator();
		// 		std::vector<VkDescriptorBufferInfo> vecBufferInfo;
		// 		vecBufferInfo.reserve(32);
		// 		std::vector<VkWriteDescriptorSet> vecWrites;
		// 
		// 		//UniformChunkWriteData writeData;
		// 		for (auto& bindingPoint : bindingPoints)
		// 		{
		// 			// allocation size is the `MaxFlightCount` times size of the binding point size
		// 			// see the `UniformStaticAllocator` implementation
		// 			UBOAllocation allocation = uniformAllocator.alloc(bindingPoint.size);
		// 			VkDescriptorBufferInfo bufferInfo; {
		// 				bufferInfo.buffer = (const VkBuffer&)*allocation.buffer;
		// 				bufferInfo.offset = 0;// allocation.offset;
		// 				bufferInfo.range = allocation.capacity;
		// 			}
		// 			vecBufferInfo.push_back(bufferInfo);
		// 			VkWriteDescriptorSet writeDescriptorSet = {}; {
		// 				writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		// 				writeDescriptorSet.pNext = nullptr;
		// 				writeDescriptorSet.dstSet = m_activeDescriptor.set;
		// 				writeDescriptorSet.dstArrayElement = 0;
		// 				writeDescriptorSet.descriptorCount = 1;
		// 				writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		// 				writeDescriptorSet.pBufferInfo = &vecBufferInfo.back();
		// 				writeDescriptorSet.pImageInfo = nullptr;
		// 				writeDescriptorSet.pTexelBufferView = nullptr;
		// 				writeDescriptorSet.dstBinding = bindingPoint.binding;
		// 			}
		// 			vecWrites.push_back(writeDescriptorSet);
		// 			// we should also store the `MaxFlightCount` frame's dynamic offsets
		// 			for (uint32_t i = 0; i < MaxFlightCount; ++i)
		// 			{
		// 				auto& vecOffsets = m_dynamicOffsets[i];
		// 				vecOffsets.push_back(static_cast<uint32_t>(allocation.capacity / MaxFlightCount * i) + static_cast<uint32_t>(allocation.offset));
		// 			}
		// 			//
		// 			m_vecUBOChunks.resize(m_vecUBOChunks.size() + 1);
		// 			m_vecUBOChunks.back().binding = bindingPoint.binding;
		// 			m_vecUBOChunks.back().uniform = allocation;
		// 		}
		// 
		// 		vkUpdateDescriptorSets(context->getDevice(), static_cast<uint32_t>(vecWrites.size()), vecWrites.data(), 0, nullptr);

	}

}