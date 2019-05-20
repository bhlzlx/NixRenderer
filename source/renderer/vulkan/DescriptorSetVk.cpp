#include "DescriptorSetVk.h"
#include "ContextVk.h"
#include "MaterialVk.h"
#include "BufferVk.h"
#include "TextureVk.h"
#include "QueueVk.h"
#include "SwapchainVk.h"
#include <algorithm>
#include <utility>

namespace nix {

	VkSampler GetSampler(const SamplerState& _state);

	VkDescriptorSet DescriptorSetPool::allocateDescriptorSet(MaterialVk* _material, uint32_t _index, uint32_t& _poolIndex) {
		VkDescriptorSetLayout layout = _material->getDescriptorSetLayout(_index);
		if (layout == VK_NULL_HANDLE) {
			return;
		}
		uint32_t setIndex = _index;
		VkDescriptorSet set = VK_NULL_HANDLE;
		uint32_t poolIndex = 0;
		//
		bool rst = false;
		for (size_t poolIndex = 0; poolIndex < m_descriptorChunks.size(); ++poolIndex)
		{
			auto& pool = m_descriptorChunks[poolIndex];
			if (pool.available()) {
				set = pool.allocate(m_device, layout);
				if (set != VK_NULL_HANDLE) {
					_poolIndex = poolIndex;
					break;
				}
			}
		}
		if (VK_NULL_HANDLE == set)
		{
			m_descriptorChunks.push_back(DescriptorSetPoolChunk());
			auto& pool = m_descriptorChunks.back();
			pool.initialize();
			set = pool.allocate(m_device, layout);
			_poolIndex = m_descriptorChunks.size() - 1;
			assert(rst && "must be true!");
		}
		return set;
	}

	ArgumentVk* DescriptorSetPool::allocateArgument(MaterialVk* _material, uint32_t _descIndex)
	{
		ArgumentVk* argument = new ArgumentVk();
		argument->m_descriptorSets[0] = allocateDescriptorSet(_material, _descIndex, argument->m_descriptorSetPools[0]);
		argument->m_descriptorSets[1] = allocateDescriptorSet(_material, _descIndex, argument->m_descriptorSetPools[1]);
		argument->m_descriptorSetIndex = _descIndex;
		argument->m_activeIndex = 0;
		return argument;
	}

	void DescriptorSetPool::free(ArgumentVk* _argument)
	{
		for (uint32_t i = 0; i < 2; ++i) {
			m_descriptorChunks[_argument->m_descriptorSetPools[i]].free( m_device, _argument->m_descriptorSets[i] );
		}
		delete _argument;
	}

	void DescriptorSetVk::setUniform(size_t _index, const void * _data, size_t _offset, size_t _size)
	{
		// a static allocated uniform should handle `MaxFlightCount` frame's uniform data
		// so when we update the uniform buffer, we should ensure which frame's data to update
		// when bind the descriptor set, we also choose which dynamic offsets to bind
		auto context = (ContextVk*)GetContextVulkan();
		uint32_t flightIndex = context->getSwapchain()->getFlightIndex();
		m_vecUBOChunks[_index].uniform.buffer->writeDataImmediatly(_data, _size, m_dynamicOffsets[flightIndex][_index] + _offset);
	}

	void DescriptorSetVk::setSampler(size_t _binding, const SamplerState& _samplerState, TextureVk* _texture)
	{
		SamplerWriteData samplerUpdate = {
			(uint32_t)_binding, _samplerState, _texture
		};
		for (auto& update : m_vecSamplerData)
		{
			if (update.binding == _binding) {
				update = samplerUpdate;
				m_needUpdate = true;
				return;
			}
		}
		m_vecSamplerData.push_back(samplerUpdate);
		m_needUpdate = true;
	}

	void DescriptorSetVk::performUpdates()
	{
		if (!m_needUpdate)
			return;
		std::vector< VkWriteDescriptorSet > vecWrites;
		std::vector< VkDescriptorBufferInfo > vecBufferInfo;
		std::vector< VkDescriptorImageInfo > vecImageInfo;

		vecWrites.reserve(64);
		vecBufferInfo.reserve(32);
		vecImageInfo.reserve(32);


		// pre-define  the write structure
		VkWriteDescriptorSet writeDescriptorSet = {}; {
			writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSet.pNext = nullptr;
			writeDescriptorSet.dstSet = m_backupDescriptor.set;
			writeDescriptorSet.dstArrayElement = 0;
			writeDescriptorSet.descriptorCount = 1;
			writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;// | VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			writeDescriptorSet.pBufferInfo = nullptr;
			writeDescriptorSet.pImageInfo = nullptr;
			writeDescriptorSet.pTexelBufferView = nullptr;
			writeDescriptorSet.dstBinding = 0;
		}
		//VkDescriptorImageInfo imageInfo;
		for (auto& ubo : m_vecUBOChunks)
		{
			vecBufferInfo.push_back(VkDescriptorBufferInfo());
			VkDescriptorBufferInfo& bufferInfo = vecBufferInfo.back();
			bufferInfo.buffer = (const VkBuffer&)(*ubo.uniform.buffer);
			bufferInfo.offset = 0;
			bufferInfo.range = ubo.uniform.capacity;
			writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
			writeDescriptorSet.dstBinding = ubo.binding;
			writeDescriptorSet.pBufferInfo = &bufferInfo;
			vecWrites.push_back(writeDescriptorSet);
		}
		for (auto& sampler : m_vecSamplerData)
		{
			vecImageInfo.push_back(VkDescriptorImageInfo());
			VkDescriptorImageInfo& imageInfo = vecImageInfo.back();
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = sampler.texture->getImageView();
			imageInfo.sampler = GetSampler(sampler.samplerState);
			writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			writeDescriptorSet.dstBinding = sampler.binding;
			writeDescriptorSet.pImageInfo = &imageInfo;
			vecWrites.push_back(writeDescriptorSet);
		}
		auto context = (ContextVk*)GetContextVulkan();
		vkUpdateDescriptorSets( context->getDevice(), static_cast<uint32_t>(vecWrites.size()), vecWrites.data(), 0, nullptr);
		// swap the `active & backup` descriptor
		DescriptorSet temp = m_activeDescriptor;
		m_activeDescriptor = m_backupDescriptor;
		m_backupDescriptor = temp;
		//
		m_needUpdate = false;
	}

	bool DescriptorSetVk::assignUniformObjects()
	{
		assert(m_vecUBOChunks.size() == 0);
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

	void DescriptorSetVk::bind(VkCommandBuffer _cmdbuff, uint32_t _flightIndex)
	{
		// update if need!
		performUpdates();
		//
		/*for (auto& sampler : m_vecSamplerData) {
			sampler.texture->transformImageLayout(_cmdbuff, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}*/
		//
		vkCmdBindDescriptorSets(
			_cmdbuff,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			m_pipeline->getPipelineLayout(),
			this->m_activeDescriptor.id,
			1,
			&m_activeDescriptor.set,
			static_cast<uint32_t>(m_dynamicOffsets[_flightIndex].size()), // uniform's dynamic offsets
			m_dynamicOffsets[_flightIndex].data()
		);
	}

	DescriptorSetVk::~DescriptorSetVk()
	{
	}

	void DescriptorSetVk::release()
	{
		// cleanup the descriptor set
		// descriptor set only handles some uniform allocation
		// so we just free the uniform allocations
		auto context = (ContextVk*)GetContextVulkan();
		auto& uboAllocator = context->getUBOAllocator();
		for (auto& alloc : m_vecUBOChunks) {
			uboAllocator.free(alloc.uniform);
		}
		m_vecUBOChunks.clear();
		//
		delete this;
	}

	bool DescriptorSetVk::getUniform(const char * _name, uint32_t& index_, uint32_t& offset_)
	{
		return m_pipeline->getUniformMember(_name, m_activeDescriptor.id, index_, offset_);
	}

	bool DescriptorSetVk::getSampler(const char * _name, uint32_t& binding_)
	{
		return m_pipeline->getSampler(_name, m_activeDescriptor.id, binding_);
	}

	void DescriptorSetPoolChunk::initialize()
	{
		// alloc new pool
		VkDescriptorPoolSize subResPools[2];
		// fill create info
		subResPools[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		subResPools[0].descriptorCount = ChunkUniformBlockCount;
		subResPools[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		subResPools[1].descriptorCount = ChunkSamplerCount;
		VkDescriptorPoolCreateInfo descriptorPoolInfo = {}; {
			descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			descriptorPoolInfo.pNext = nullptr;
			descriptorPoolInfo.poolSizeCount = sizeof(subResPools) / sizeof(VkDescriptorPoolSize);
			descriptorPoolInfo.pPoolSizes = subResPools;
			descriptorPoolInfo.maxSets = ChunkSamplerCount / 2;
		}
		// create!
		auto context = (ContextVk*)GetContextVulkan();
		VkResult rst = vkCreateDescriptorPool(context->getDevice(), &descriptorPoolInfo, nullptr, &m_pool);
		if (rst != VK_SUCCESS) {
			assert(false);
		}
	}

	VkDescriptorSet DescriptorSetPoolChunk::allocate(VkDevice _device, VkDescriptorSetLayout _descSetLayout) {
		VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
		// setup create info
		VkDescriptorSetAllocateInfo inf = {}; {
			inf.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			inf.pNext = nullptr;
			inf.descriptorPool = m_pool;
			inf.descriptorSetCount = 1;
			inf.pSetLayouts = &_descSetLayout;
		}
		// allocate!!!
		auto rst = vkAllocateDescriptorSets( _device, &inf, &descriptorSet );
		// get the result
		if (rst == VK_SUCCESS) 
		{
			return descriptorSet;
		}
		else
		{
			m_state = 0;
			return VK_NULL_HANDLE;
		}
	}

	void DescriptorSetPoolChunk::free( VkDevice _device, VkDescriptorSet _descSet) {
		VkResult rst = vkFreeDescriptorSets( _device, m_pool, 1, &_descSet );
		assert(rst == VK_SUCCESS);
		m_state = 1;
	}
}