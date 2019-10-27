#include "TextureVk.h"
#include "ContextVk.h"
#include "BufferVk.h"
#include "QueueVk.h"
#include "DeferredDeletor.h"
#include "vkhelper/helper.h"
#include "TypemappingVk.h"
#include "BufferAllocator.h"
#include <cassert>

namespace Nix {
	//
	ITexture* ContextVk::createTexture(const TextureDescription& _desc, TextureUsageFlags _usage) {
		return TextureVk::createTexture(this, VK_NULL_HANDLE, VK_NULL_HANDLE, _desc, _usage);
	}

	ITexture* ContextVk::createTextureDDS(const void* _data, size_t _length) {
		return TextureVk::createTextureDDS(this, _data, _length);
	}

	ITexture* ContextVk::createTextureKTX(const void* _data, size_t _length) {
		return TextureVk::createTextureKTX(this, _data, _length);
	}

	void TextureVk::updateSubData(const void * _data, size_t _length, const TextureRegion& _region)
	{
		m_context->getGraphicsQueue()->updateTexture(this, _region, _data, _length);
	}

	void TextureVk::uploadSubData(const BufferImageUpload& _upload)
	{
		m_context->getUploadQueue()->uploadTexture(this, _upload);
	}

	// 	void TextureVk::setSubData(const void * _data, size_t _length, const TextureRegion& _baseMipRegion, uint32_t _mipCount)
	// 	{
	// 		m_context->getGraphicsQueue()->updateTexture(this, _baseMipRegion, _mipCount, _data, _length);
	// 	}

	void TextureVk::release()
	{
		auto deletor = GetDeferredDeletor();
		deletor.destroyResource(this);
	}

	TextureVk::~TextureVk()
	{
		if (m_ownership& OwnImage) {
			vmaDestroyImage(m_context->getVmaAllocator(), m_image, m_allocation);
		}
		if (m_ownership& OwnImageView) {
			vkDestroyImageView(m_context->getDevice(), m_imageView, nullptr);
		}
	}

	VkImage TextureVk::getImage() const {
		return m_image;
	}
	VkImageView TextureVk::getImageView() const {
		return m_imageView;
	}

	VkImageLayout TextureVk::getImageLayout() const
	{
		return m_imageLayout;
	}

	void TextureVk::transformImageLayout(VkCommandBuffer _cmdBuffer, VkImageLayout _newLayout)
	{
		if (m_imageLayout == _newLayout) {
			return;
		}
		VkAccessFlags dstAccessFlag;
		VkPipelineStageFlags dstStageFlag;
		vkhelper::getImageAcessFlagAndPipelineStage(_newLayout, dstAccessFlag, dstStageFlag);
		// attentions !!!!
		// for some `Android Platform`
		// VK_REMAINING_MIP_LEVELS & VK_REMAINING_ARRAY_LAYERS will result in `Crash issues`!
		VkImageMemoryBarrier barrier = {
			VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, // sType 
			nullptr, // pNext
			m_accessFlags,
			dstAccessFlag,
			m_imageLayout, // oldLayout
			_newLayout, // newLayout 
			VK_QUEUE_FAMILY_IGNORED, // srcQueueFamilyIndex 
			VK_QUEUE_FAMILY_IGNORED, // dstQueueFamilyIndex 
			m_image, // image 
			{ // subresourceRange 
				m_aspectFlags, // aspectMask 
				0, // baseMipLevel 
				m_descriptor.mipmapLevel,//,VK_REMAINING_MIP_LEVELS, // levelCount
				0, // baseArrayLayer
				m_descriptor.depth//VK_REMAINING_ARRAY_LAYERS // layerCount
			}
		};
		vkCmdPipelineBarrier(_cmdBuffer, m_pipelineStages, dstStageFlag,
			0, // VkDependencyFlags : it is out of an render pass!
			0, nullptr,
			0, nullptr,
			1, &barrier
		);
		m_pipelineStages = dstStageFlag;
		m_accessFlags = dstAccessFlag;
		m_imageLayout = _newLayout;
	}

	Nix::TextureVk* TextureVk::createTexture(ContextVk* _context, VkImage _image, VkImageView _imageView, TextureDescription _desc, TextureUsageFlags _usage)
	{
		VkFormat format = NixFormatToVk(_desc.format);
		VkImageAspectFlags aspectMask = 0;
		//
		if (_usage == 0)
		{
			if (vkhelper::isDepthFormat(format))
			{
				_usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
				assert(_desc.type != TextureCube);
			}
			else
			{
				if (vkhelper::isCompressedFormat(format)) {
					_usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
				}
				else {
					_usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
				}

			}
		}
		TextureVk::TexResOwnershipFlags ownershipFlags = 0;
		VmaAllocation allocation = nullptr;
		if (!_image)
		{
			ownershipFlags |= TextureVk::TexResOwnershipFlagBits::OwnImage;
			VkImageCreateFlags imageFlags = 0;
			VkImageType imageType = VK_IMAGE_TYPE_1D;
			if (_desc.type == TextureCube || _desc.type == TextureCubeArray) {
				imageFlags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
				imageType = VK_IMAGE_TYPE_2D;
			}
			else if (_desc.type == Texture2DArray) {
				imageType = VK_IMAGE_TYPE_2D;
			}
			else if (_desc.type == Texture2D) {
				imageType = VK_IMAGE_TYPE_2D;
			}

			VkImageCreateInfo info; {
				info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
				info.pNext = nullptr;
				info.flags = imageFlags;
				info.imageType = imageType;
				info.format = NixFormatToVk(_desc.format);
				info.extent = {
					_desc.width, _desc.height, 1
				};
				info.mipLevels = _desc.mipmapLevel;
				info.arrayLayers = 1;
				info.samples = VK_SAMPLE_COUNT_1_BIT;
				info.tiling = VK_IMAGE_TILING_OPTIMAL;
				info.usage = _usage;
				info.sharingMode = _context->getQueueFamilies().size() > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
				info.queueFamilyIndexCount = _context->getQueueFamilies().size();
				info.pQueueFamilyIndices = _context->getQueueFamilies().data();
				info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			}
			info.arrayLayers = _desc.depth;

			VmaAllocationCreateInfo allocInfo = {}; {
				allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
			};
			VkResult rst = vmaCreateImage(_context->getVmaAllocator(), &info, &allocInfo, &_image, &allocation, nullptr);
			if (rst != VK_SUCCESS) {
				return nullptr;
			}
		}

		if (vkhelper::isDepthFormat(format))
			aspectMask |= VK_IMAGE_ASPECT_DEPTH_BIT;
		if (vkhelper::isStencilFormat(format))
			aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		if (!aspectMask)
			aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		if (!_imageView) {
			ownershipFlags |= TextureVk::TexResOwnershipFlagBits::OwnImageView;
			// create image view
			VkImageViewCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = _image;
			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			//
			createInfo.subresourceRange.aspectMask = aspectMask;

			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = _desc.mipmapLevel;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = _desc.depth;

			createInfo.pNext = nullptr;
			switch (_desc.type)
			{
			case TextureType::Texture2D:
				createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D; break;
			case TextureType::Texture2DArray:
				createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY; break;
			case TextureType::TextureCube:
				createInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE; break;
			case TextureType::TextureCubeArray:
				createInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY; break;
			case TextureType::Texture3D:
				createInfo.viewType = VK_IMAGE_VIEW_TYPE_3D; break;
			}
			createInfo.format = format;
			createInfo.flags = 0;
			// texture aspect should have all aspect information
			// but the image view should not have the stencil aspect
			createInfo.subresourceRange.aspectMask &= ~VK_IMAGE_ASPECT_STENCIL_BIT;
			VkDevice device = _context->getDevice();
			VkResult rst = vkCreateImageView(device, &createInfo, nullptr, &_imageView);
			assert(VK_SUCCESS == rst);
		}
		//
		TextureVk* texture = new TextureVk();
		texture->m_context = _context;
		texture->m_image = _image;
		texture->m_imageView = _imageView;
		texture->m_allocation = allocation;
		texture->m_descriptor = _desc;
		texture->m_aspectFlags = aspectMask;
		texture->m_ownership = ownershipFlags;
		texture->m_accessFlags = 0;
		texture->m_pipelineStages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		texture->m_imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		/*if (_usage & TextureUsageColorAttachment || _usage & TextureUsageDepthStencilAttachment) {
		}
		else {
			_context->getUploadQueue()->tranformImageLayout(texture, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}*/

		return texture;
	}
}
