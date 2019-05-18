#include <NixRenderer.h>
#include "../vkinc.h"
#include <vector>
#include <string>
#include <cassert>

namespace vkhelper
{
	NIX_API_DECL uint32_t getMemoryType(VkPhysicalDevice _device, uint32_t _memTypeBits, VkMemoryPropertyFlags properties)
	{
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(_device, &memProperties);
		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
		{
			if (_memTypeBits & (1 << i))
			{
				if ((memProperties.memoryTypes[i].propertyFlags & properties) == properties)
				{
					return i;
				}
			}
		}
		throw std::runtime_error("failed to find suitable memory type!");
		return 0;
	}

	NIX_API_DECL VkBool32 isDepthFormat(VkFormat _format)
	{
		switch (_format)
		{
		case VK_FORMAT_D16_UNORM:
		case VK_FORMAT_X8_D24_UNORM_PACK32:
		case VK_FORMAT_D32_SFLOAT:
		case VK_FORMAT_D16_UNORM_S8_UINT:
		case VK_FORMAT_D24_UNORM_S8_UINT:
		case VK_FORMAT_D32_SFLOAT_S8_UINT:
			return VK_TRUE;
		default:
			return VK_FALSE;
		}
	}

	NIX_API_DECL VkBool32 isStencilFormat(VkFormat _format)
	{
		switch (_format)
		{
		case VK_FORMAT_S8_UINT:
		case VK_FORMAT_D16_UNORM_S8_UINT:
		case VK_FORMAT_D24_UNORM_S8_UINT:
		case VK_FORMAT_D32_SFLOAT_S8_UINT:
			return VK_TRUE;
		default:
			return VK_FALSE;
		}
	}

	NIX_API_DECL void getImageAcessFlagAndPipelineStage(VkImageLayout _layout, VkAccessFlags& _flags, VkPipelineStageFlags& _stages)
	{
		switch (_layout)
		{
		case VK_IMAGE_LAYOUT_UNDEFINED:
		case VK_IMAGE_LAYOUT_PREINITIALIZED:
		case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
		{
			_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			_flags = 0;
			break;
		}
		case VK_IMAGE_LAYOUT_GENERAL:
		{
			_stages = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			_flags = VK_ACCESS_FLAG_BITS_MAX_ENUM;
			break;
		}
		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		{
			_stages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			_flags = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			break;
		}
		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL:
		case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL:
		{
			_stages = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			_flags = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
			break;
		}
		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
		{
			_stages = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			_flags = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
			break;
		}
		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		{
			_stages = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			_flags = VK_ACCESS_SHADER_READ_BIT;
			break;
		}
		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
		{
			_stages = VK_PIPELINE_STAGE_TRANSFER_BIT;
			_flags = VK_ACCESS_TRANSFER_WRITE_BIT;
			break;
		}
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		{
			_stages = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			_flags = VK_ACCESS_SHADER_WRITE_BIT;
			break;
		}
		default:
			assert(false);
			break;
		}
	}
}
