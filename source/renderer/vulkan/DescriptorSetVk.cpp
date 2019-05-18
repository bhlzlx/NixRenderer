#include "DescriptorSetVk.h"
#include "ContextVk.h"
#include "PipelineVk.h"
#include "BufferVk.h"
#include "TextureVk.h"
#include "QueueVk.h"
#include "SwapchainVk.h"
#include <algorithm>
#include <utility>

namespace nix {

	VkSampler GetSampler(const SamplerState& _state);

	std::vector< DescriptorSet > DescriptorSetPool::allocOneBatch( PipelineVk* _pipeline ) {
		auto& descriptorSetLayouts = _pipeline->GetDescriptorSetLayouts();
		if (!descriptorSetLayouts.size()) {
			return std::move(std::vector< DescriptorSet >()); // return empty descriptor set
		}
		std::vector<VkDescriptorSetLayout> layouts;
		std::vector<uint32_t> layoutIDs;
		std::vector<VkDescriptorSet> sets;
		uint32_t poolIndex = 0;
		//
		for (size_t i = 0; i < descriptorSetLayouts.size(); ++i) {
			layouts.push_back(descriptorSetLayouts[i].layout);
			layoutIDs.push_back(descriptorSetLayouts[i].id);
			sets.push_back(VK_NULL_HANDLE);
		}
		bool rst = false;
		for (auto& pool : m_vecMiniPools) {
			if (pool.available()) {
				rst = pool.allocate(layouts.data(), layouts.size(), sets.data());
				if (rst) {
					break;
				}
			}
			++poolIndex;
		}
		if (!rst) {
			m_vecMiniPools.push_back(DescriptorSetPoolChunk());
			auto& pool = m_vecMiniPools.back();
			pool.initialize();
			rst = pool.allocate(layouts.data(), layouts.size(), sets.data());
			assert(rst && "must be true!");
		}
		// arrange the descriptor set information
		std::vector< DescriptorSet > descriptorSetInfos;
		for (size_t i = 0; i < descriptorSetLayouts.size(); ++i) {
			DescriptorSet ds;
			ds.poolIndex = poolIndex;
			ds.set = sets[i];
			ds.id = layoutIDs[i];
			descriptorSetInfos.push_back(ds);
		}
		return std::move(descriptorSetInfos);
	}

	DescriptorSet DescriptorSetPool::allocOne( PipelineVk* _effect, uint32_t _setID)
	{
		auto& descriptorSetLayouts = _effect->GetDescriptorSetLayouts();
		DescriptorSetLayout layout = { VK_NULL_HANDLE, uint32_t(-1) };
		for (auto& l : descriptorSetLayouts)
		{
			if (l.id == _setID) {
				layout.id = l.id;
				layout.layout = l.layout;
				break;
			}
		}
		if (layout.layout == VK_NULL_HANDLE) {
			assert(false && "cannot find the set id!");
			return DescriptorSet();
		}
		VkDescriptorSet set;
		uint32_t poolIndex = 0;
		bool rst = false;
		for (auto& pool : m_vecMiniPools) {
			if (pool.available()) {
				rst = pool.allocate(&layout.layout, 1, &set);
				if (rst) {
					break;
				}
			}
			++poolIndex;
		}
		if (!rst) {
			m_vecMiniPools.push_back(DescriptorSetPoolChunk());
			auto& pool = m_vecMiniPools.back();
			pool.initialize();
			rst = pool.allocate(&layout.layout, 1, &set);
			assert(rst && "must be true!");
		}
		// arrange the descriptor set information
		DescriptorSet setData;
		setData.poolIndex = poolIndex;
		setData.set = set;
		setData.id = _setID;
		return setData;
	}

	std::vector<DescriptorSetVk*> DescriptorSetPool::alloc(PipelineVk* _pipeline)
	{
		auto activeSet = allocOneBatch(_pipeline);
		auto backupSet = allocOneBatch(_pipeline);
		std::vector<DescriptorSetVk*> descriptorSets;
		for (size_t i = 0; i < activeSet.size(); ++i) {
			DescriptorSetVk* pSet = new DescriptorSetVk();
			pSet->m_pipeline = _pipeline;
			pSet->m_activeDescriptor = activeSet[i];
			pSet->m_backupDescriptor = backupSet[i];
			descriptorSets.push_back(pSet);
			//
			pSet->assignUniformObjects();
		}
		return descriptorSets;
	}

	DescriptorSetVk* DescriptorSetPool::alloc( PipelineVk* _pipeline, int _setId)
	{
		auto activeSet = allocOne(_pipeline, _setId);
		auto backupSet = allocOne(_pipeline, _setId);
		DescriptorSetVk* set = new DescriptorSetVk();
		set->m_activeDescriptor = activeSet;
		set->m_backupDescriptor = backupSet;
		set->m_pipeline = _pipeline;
		set->assignUniformObjects();
		return set;
	}

	void DescriptorSetPool::free(const std::vector<DescriptorSetVk*>& _sets)
	{
		for (auto& set : _sets)
		{
			m_vecMiniPools[set->m_activeDescriptor.poolIndex].free(&set->m_activeDescriptor.set, 1);
			m_vecMiniPools[set->m_backupDescriptor.poolIndex].free(&set->m_backupDescriptor.set, 1);
		}
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