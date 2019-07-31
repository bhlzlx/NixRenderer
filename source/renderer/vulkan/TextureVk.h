#include "VkInc.h"
#include <NixRenderer.h>
#include <vk_mem_alloc.h>


// different from buffer object destroy
// the texture `release` method will not delete the object right now
// pass the pointer to the `deferred deleter`
// and the deleter will destroy it when not used any longer!

namespace Nix {
	class ContextVk;

	class NIX_API_DECL TextureVk : public ITexture {
		friend class ContextVk;
		friend class RenderPassVk;
		friend class CommandBufferVk;
		friend class RenderPassSwapchainVk;
		friend class UploadQueueVk;
	private:
		enum TexResOwnershipFlagBits {
			OwnImage = 0x1,
			OwnImageView = 0x2
		};
		typedef uint32_t TexResOwnershipFlags;

		ContextVk*				m_context;
		TextureDescription		m_descriptor;
		VkImage					m_image;
		VkImageView				m_imageView;
		VmaAllocation			m_allocation;
		TexResOwnershipFlags	m_ownership;
		// Vulkan state flags
		VkImageAspectFlags		m_aspectFlags;
		VkAccessFlags			m_accessFlags;
		VkPipelineStageFlags	m_pipelineStages;
		VkImageLayout			m_imageLayout;
	private:
		void setImageLayout(VkImageLayout _imageLayout) {
			m_imageLayout = _imageLayout;
		}
	public:
		~TextureVk();
		virtual const TextureDescription& getDesc() const {
			return m_descriptor;
		}
		// need graphics queue support
		// can update whole image,once with a chunk of memory
		virtual void updateSubData(const void * _data, size_t _length, const TextureRegion& _region) override;
		virtual void uploadSubData(const BufferImageUpload& _upload) override;
		//virtual void setSubData(const void * _data, size_t _length, const TextureRegion& _baseMipRegion, uint32_t _mipCount) override;
		// deferred deleter
		virtual void release() override;
		//
		VkImage getImage() const;
		VkImageView getImageView() const;
		VkImageLayout getImageLayout() const;
		void transformImageLayout(VkCommandBuffer _cmdBuffer, VkImageLayout _layout);
		//
		static TextureVk* createTexture( ContextVk* _context, VkImage _image, VkImageView _imageView, TextureDescription _desc, TextureUsageFlags _usage );
		static TextureVk* createTextureDDS(ContextVk* _context, const void * _data, size_t _length);
		static TextureVk* createTextureKTX(ContextVk* _context, const void * _data, size_t _length);
	};
}