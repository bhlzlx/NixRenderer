#include "DescriptorSetVk.h"
#include "ContextVk.h"
#include "MaterialVk.h"
#include "BufferVk.h"
#include "TextureVk.h"
#include "QueueVk.h"
#include "SwapchainVk.h"
#include <algorithm>
#include <utility>

namespace Nix {

	VkDescriptorSet ArgumentAllocator::allocateDescriptorSet( MaterialVk* _material, uint32_t _index, uint32_t& _poolIndex ) {
		const ArgumentLayoutExt& layout = _material->getDescriptorSetLayout(_index);
		if (layout.m_setLayout == VK_NULL_HANDLE) {
			return VK_NULL_HANDLE;
		}
		uint32_t setIndex = _index;
		VkDescriptorSet set = VK_NULL_HANDLE;
		uint32_t poolIndex = 0;
		//
		bool rst = false;
		for (uint32_t poolIndex = 0; poolIndex < m_descriptorChunks.size(); ++poolIndex)
		{
			auto& pool = m_descriptorChunks[poolIndex];
			set = pool.allocate(m_context->getDevice(), _material, _index);
			if (set != VK_NULL_HANDLE) {
				_poolIndex = poolIndex;
				break;
			}
		}
		if (VK_NULL_HANDLE == set)
		{
			m_descriptorChunks.push_back(ArgumentPoolChunk());
			auto& pool = m_descriptorChunks.back();
			pool.initialize( m_context->getDevice(), &DescriptorSetPoolConstruction[0], sizeof(DescriptorSetPoolConstruction) / sizeof(VkDescriptorPoolSize));
			set = pool.allocate(m_context->getDevice(), _material, _index);
			_poolIndex = (uint32_t)m_descriptorChunks.size() - 1;
			assert( (set != VK_NULL_HANDLE) && "must be true!" );
		}
		return set;
	}

	ArgumentVk* ArgumentAllocator::allocateArgument(MaterialVk* _material, uint32_t _descIndex)
	{
		ArgumentVk* argument = new ArgumentVk();
		for (uint32_t i = 0; i < MaxFlightCount; ++i) {
			argument->m_descriptorSets[i] = allocateDescriptorSet(_material, _descIndex, argument->m_descriptorSetPools[i]);
		}
		argument->m_descriptorSetIndex = _descIndex;
		argument->m_activeIndex = 0;
		argument->m_material = _material;
		argument->m_context = _material->getContext();
		
		const ArgumentLayoutExt& argLayout = _material->m_argumentLayouts[_descIndex];
		
		uint32_t bufferCounter = 0, imageCounter = 0;
		/*
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
		VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC
		VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC
		*/
		bufferCounter += (uint32_t)argLayout.m_vecUniformBuffer.size();
		bufferCounter += (uint32_t)argLayout.m_vecStorageBuffer.size();
		//
		imageCounter += (uint32_t)argLayout.m_vecTBO.size();
		imageCounter += (uint32_t)argLayout.m_vecSampler.size();
		imageCounter += (uint32_t)argLayout.m_vecSubpassInput.size();
		
		argument->m_vecBufferInfo.resize(bufferCounter);
		/*
		VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE
		VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
		VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT
		*/
		argument->m_vecImageInfo.resize(imageCounter);
		//
		argument->m_vecDescriptorWrites.resize(imageCounter + bufferCounter);
		//
		bufferCounter = 0, imageCounter = 0;
		uint32_t writeCounter = 0;
		
		for ( auto& uniform : argLayout.m_vecUniformBuffer )
		{
			argument->m_vecDescriptorWrites[writeCounter] = {};{
				argument->m_vecDescriptorWrites[writeCounter].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				argument->m_vecDescriptorWrites[writeCounter].pNext = nullptr;
				argument->m_vecDescriptorWrites[writeCounter].dstSet = argument->m_descriptorSets[0];
				argument->m_vecDescriptorWrites[writeCounter].dstArrayElement = 0;
				argument->m_vecDescriptorWrites[writeCounter].descriptorCount = 1;
				argument->m_vecDescriptorWrites[writeCounter].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
				argument->m_vecDescriptorWrites[writeCounter].pBufferInfo = &argument->m_uniformBufferInfo[bufferCounter];
				argument->m_vecDescriptorWrites[writeCounter].pImageInfo = nullptr;
				argument->m_vecDescriptorWrites[writeCounter].pTexelBufferView = nullptr;
				argument->m_vecDescriptorWrites[writeCounter].dstBinding = uniform.binding;
			};
			++bufferCounter;
			++writeCounter;
		}
		
		for (auto& ssbo : argLayout.m_storageBufferDescriptor)
		{
			argument->m_vecDescriptorWrites[writeCounter] = {}; {
				argument->m_vecDescriptorWrites[writeCounter].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				argument->m_vecDescriptorWrites[writeCounter].pNext = nullptr;
				argument->m_vecDescriptorWrites[writeCounter].dstSet = argument->m_descriptorSets[0];
				argument->m_vecDescriptorWrites[writeCounter].dstArrayElement = 0;
				argument->m_vecDescriptorWrites[writeCounter].descriptorCount = 1;
				argument->m_vecDescriptorWrites[writeCounter].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				argument->m_vecDescriptorWrites[writeCounter].pBufferInfo = &argument->m_vecDescriptorBufferInfo[bufferCounter];
				argument->m_vecDescriptorWrites[writeCounter].pImageInfo = nullptr;
				argument->m_vecDescriptorWrites[writeCounter].pTexelBufferView = nullptr;
				argument->m_vecDescriptorWrites[writeCounter].dstBinding = ssbo.binding;
			};
			++bufferCounter;
			++writeCounter;
		}
		
		for (auto& sampler : argLayout.m_samplerImageDescriptor)
		{
			argument->m_vecDescriptorWrites[writeCounter] = {}; {
				argument->m_vecDescriptorWrites[writeCounter].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				argument->m_vecDescriptorWrites[writeCounter].pNext = nullptr;
				argument->m_vecDescriptorWrites[writeCounter].dstSet = argument->m_descriptorSets[0];
				argument->m_vecDescriptorWrites[writeCounter].dstArrayElement = 0;
				argument->m_vecDescriptorWrites[writeCounter].descriptorCount = 1;
				argument->m_vecDescriptorWrites[writeCounter].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				argument->m_vecDescriptorWrites[writeCounter].pBufferInfo = nullptr;
				argument->m_vecDescriptorWrites[writeCounter].pImageInfo = &argument->m_vecSamplerImageInfo[imageCounter];
				argument->m_vecDescriptorWrites[writeCounter].pTexelBufferView = nullptr;
				argument->m_vecDescriptorWrites[writeCounter].dstBinding = sampler.binding;
			};
			++imageCounter;
			++writeCounter;
		}

// 		for (auto& sampler : argLayout.m_texelBufferDescriptor)
// 		{
// 			argument->m_vecDescriptorWrites[writeCounter] = {}; {
// 				argument->m_vecDescriptorWrites[writeCounter].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
// 				argument->m_vecDescriptorWrites[writeCounter].pNext = nullptr;
// 				argument->m_vecDescriptorWrites[writeCounter].dstSet = argument->m_descriptorSets[0];
// 				argument->m_vecDescriptorWrites[writeCounter].dstArrayElement = 0;
// 				argument->m_vecDescriptorWrites[writeCounter].descriptorCount = 1;
// 				argument->m_vecDescriptorWrites[writeCounter].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
// 				argument->m_vecDescriptorWrites[writeCounter].pBufferInfo = nullptr;
// 				argument->m_vecDescriptorWrites[writeCounter].pImageInfo = &argument->m_vecSamplerImageInfo[imageCounter];
// 				argument->m_vecDescriptorWrites[writeCounter].pTexelBufferView = nullptr;
// 				argument->m_vecDescriptorWrites[writeCounter].dstBinding = sampler.binding;
// 			};
// 			++imageCounter;
// 			++writeCounter;
// 		}
		return argument;
	}

	void ArgumentAllocator::free(ArgumentVk* _argument)
	{
		for (uint32_t i = 0; i < MaxFlightCount; ++i) {
			m_descriptorChunks[_argument->m_descriptorSetPools[i]].free(m_context->getDevice(), _argument->m_descriptorSets[i] );
		}
		delete _argument;
	}

	ArgumentAllocator::~ArgumentAllocator() {

	}

	inline ArgumentPoolChunk::ArgumentPoolChunk() : m_pool(VK_NULL_HANDLE) {
	}

	void ArgumentPoolChunk::initialize( VkDevice _device, const VkDescriptorPoolSize* _pools, uint32_t _poolCount)
	{
		std::vector<VkDescriptorPoolSize> pools;
		for (uint32_t i = 0; i < _poolCount; ++i) {
			if (_pools[i].descriptorCount != 0) {
				pools.push_back(_pools[i]);
			}
		}
		VkDescriptorPoolCreateInfo descriptorPoolInfo = {}; {
			descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			descriptorPoolInfo.pNext = nullptr;
			descriptorPoolInfo.poolSizeCount = (uint32_t)pools.size();
			descriptorPoolInfo.pPoolSizes = pools.data();
			descriptorPoolInfo.maxSets = 128;
		}
		// create!
		VkResult rst = vkCreateDescriptorPool(_device, &descriptorPoolInfo, nullptr, &m_pool);
		if (rst != VK_SUCCESS) {
			assert(false);
		}
		for ( uint32_t i = 0; i<_poolCount; ++i ) {
			m_freeTable[_pools[i].type].descriptorCount = _pools[i].descriptorCount;
			m_freeTable[_pools[i].type].type = _pools[i].type;
		}
		m_device = _device;
	}

	void ArgumentPoolChunk::cleanup()
	{
		vkDestroyDescriptorPool(m_device, m_pool, nullptr);
	}

	VkDescriptorSet ArgumentPoolChunk::allocate(VkDevice _device, MaterialVk* _material, uint32_t _argumentIndex) {
		VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
		VkResult rst = VK_ERROR_OUT_OF_HOST_MEMORY;
		const ArgumentLayout& argumentLayout = _material->getDescriptorSetLayout(_argumentIndex);
		if ( m_freeTable[VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC].descriptorCount >= argumentLayout.m_uniformBlockDescriptor.size()
			&& m_freeTable[VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC].descriptorCount >= argumentLayout.m_storageBufferDescriptor.size()
			&& m_freeTable[VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER].descriptorCount >= argumentLayout.m_samplerImageDescriptor.size()
			//&& m_freeTable[VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER].descriptorCount >= argumentLayout.m_texelBufferDescriptor.size()
		)
		{
			// setup create info
			VkDescriptorSetAllocateInfo inf = {}; {
				inf.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
				inf.pNext = nullptr;
				inf.descriptorPool = m_pool;
				inf.descriptorSetCount = 1;
				inf.pSetLayouts = &argumentLayout.m_descriptorSetLayout;
			}
			// allocate!!!
			rst = vkAllocateDescriptorSets(_device, &inf, &descriptorSet);
			// get the result
			if (rst == VK_SUCCESS) {
				m_freeTable[VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC].descriptorCount -= (uint32_t)argumentLayout.m_uniformBlockDescriptor.size();
				m_freeTable[VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC].descriptorCount -= (uint32_t)argumentLayout.m_storageBufferDescriptor.size();
				m_freeTable[VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER].descriptorCount -= (uint32_t)argumentLayout.m_samplerImageDescriptor.size();
				//m_freeTable[VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER].descriptorCount -= (uint32_t)argumentLayout.m_texelBufferDescriptor.size();
				return descriptorSet;
			}
			else {
				return VK_NULL_HANDLE;
			}
		}
		else
		{
			return VK_NULL_HANDLE;
		}

	}

	void ArgumentPoolChunk::free( VkDevice _device, VkDescriptorSet _descSet) {
		VkResult rst = vkFreeDescriptorSets( _device, m_pool, 1, &_descSet );
		assert(rst == VK_SUCCESS);
	}

	inline ArgumentPoolChunk::~ArgumentPoolChunk() {

	}

	void ArgumentAllocator::initialize(ContextVk* _context) {
		m_context = _context;
	}

	void ArgumentAllocator::cleanup()
	{
		for (auto& chunk : this->m_descriptorChunks) {
			chunk.cleanup();
		}
		m_descriptorChunks.clear();
	}
}