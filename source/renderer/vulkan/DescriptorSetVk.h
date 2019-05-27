#pragma once
#include <NixRenderer.h>
#include "vkinc.h"
#include <vector>
#include <list>
#include <map>
#include <array>
#include <vector>
#include "ArgumentVk.h"
#include <NixRenderer.h>

namespace nix {

	class MaterialVk;
	class TextureVk;
	class BufferVk;

	constexpr std::pair<VkDescriptorType, uint32_t> DescriptorSetPoolConstruction[] = {
		{ VK_DESCRIPTOR_TYPE_SAMPLER					, 0 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER		, 128 * MaxFlightCount },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE				, 0 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE				, 0 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER		, 0 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER		, 0 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER				, 0 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER				, 16 * MaxFlightCount },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC		, 512 * MaxFlightCount },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC		, 16 * MaxFlightCount },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT			, 16 * MaxFlightCount }
	};

	class NIX_API_DECL DescriptorSetPoolChunk {
	private:
		VkDescriptorPool	m_pool;
		uint8_t				m_state; // 0 : unavailable
		VkDevice			m_device;
		
	public:
		DescriptorSetPoolChunk() : m_pool(VK_NULL_HANDLE), m_state(0) {
		}
		bool available() const {
			return m_state != 0;
		}
		void initialize( VkDevice _device, DescriptorSetPoolConstruction, uint32_t _count );
		VkDescriptorSet allocate( VkDevice _device, VkDescriptorSetLayout _descSetLayout );
		void free( VkDevice _device, VkDescriptorSet _descSet );
	};

	class NIX_API_DECL DescriptorSetPool
	{
	private:
		ContextVk*								m_context;
		std::vector< DescriptorSetPoolChunk >	m_descriptorChunks;
	private:
		VkDescriptorSet allocateDescriptorSet(MaterialVk* _material, uint32_t _index, uint32_t& _poolIndex);
	public:
		DescriptorSetPool()
			:m_context( nullptr )
		{
		}
		void initialize(ContextVk* _context);
		//
		ArgumentVk* allocateArgument(MaterialVk* _material, uint32_t _descIndex);
		void free( ArgumentVk* _argument );
	};
}