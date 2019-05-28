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

	constexpr VkDescriptorPoolSize DescriptorSetPoolConstruction[] = {
		{ VK_DESCRIPTOR_TYPE_SAMPLER					, 0						},
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER		, 128 * MaxFlightCount	},
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE				, 0						},
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE				, 0						},
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER		, 0						},
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER		, 0						},
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER				, 0						},
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER				, 0						},
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC		, 512 * MaxFlightCount	},
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC		, 16 * MaxFlightCount	},
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT			, 16 * MaxFlightCount	},
	};

	class NIX_API_DECL ArgumentPoolChunk {
	private:
		VkDescriptorPool			m_pool;
		VkDevice					m_device;
		VkDescriptorPoolSize		m_freeTable[VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT];
	public:
		ArgumentPoolChunk();
		void initialize( VkDevice _device, const VkDescriptorPoolSize* _pools, uint32_t _poolCount );
		VkDescriptorSet allocate( VkDevice _device, MaterialVk* _material, uint32_t _argumentIndex );
		void free( VkDevice _device, VkDescriptorSet _descSet );
	};

	class NIX_API_DECL ArgumentAllocator
	{
	private:
		ContextVk*								m_context;
		std::vector< ArgumentPoolChunk >		m_descriptorChunks;
	private:
		VkDescriptorSet allocateDescriptorSet(MaterialVk* _material, uint32_t _index, uint32_t& _poolIndex);
	public:
		ArgumentAllocator()
			:m_context( nullptr )
		{
		}
		void initialize(ContextVk* _context);
		//
		ArgumentVk* allocateArgument(MaterialVk* _material, uint32_t _descIndex);
		void free( ArgumentVk* _argument );
	};
}