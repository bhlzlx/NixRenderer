#pragma once
#include <NixRenderer.h>
#include <map>
#include "vkinc.h"
#include <vector>
#include <cassert>

namespace Ks {

	class TextureVk;

	class KS_API_DECL AttachmentVk:public IAttachment {
		friend class ContextVk;
	private:
		ContextVk* m_context;
		TextureVk* m_texture;
		Size<uint32_t> m_size;
		KsFormat m_format;
		VkBool32 m_isRef;
	public:
		//virtual void resize(uint32_t _width, uint32_t _height) override;
		virtual const ITexture* getTexture() const override;
		virtual KsFormat getFormat() const override;
		virtual void release() override;
		~AttachmentVk() {
			delete this;
		}
		static AttachmentVk* createAttachment( ContextVk* _context, TextureVk* _texture );
		static AttachmentVk* createAttachment( ContextVk* _context, KsFormat _format, uint32_t _width, uint32_t _height);
	};

	class KS_API_DECL RenderPassVk : public IRenderPass {
		friend class ContextVk;
	private:
		ContextVk* m_context;
		RenderPassDescription m_desc;
		VkFramebuffer m_framebuffer;
		VkRenderPass m_renderPass;
		//
		AttachmentVk* m_colorAttachments[MaxRenderTarget];
		AttachmentVk* m_depthStencil;
		VkImageLayout m_colorImageLayout[MaxRenderTarget];
		VkImageLayout m_dsImageLayout;
		//
		uint8_t m_clearCount; // clear count
		VkClearValue m_clearValues[MaxRenderTarget+1]; // color & depth-stencil clear values
		//
		Size<uint32_t> m_size;
		//
		static std::map< RenderPassDescription, VkRenderPass > m_renderPassMapTable;
	public:
		RenderPassVk() {
			memset(m_colorAttachments, 0, sizeof(m_colorAttachments));
			memset(m_colorImageLayout, 0, sizeof(m_colorImageLayout));
			memset(m_clearValues, 0, sizeof(m_clearValues));
			m_clearValues[1].depthStencil.depth = 1.0f;
		}
		~RenderPassVk();
		//
		bool begin();
		void resize(uint32_t _width, uint32_t _height);
		void end(); 
		void setClear( const RpClear& _clear );
		//
		void release() {
		}
		//
		const RenderPassDescription& getDescption() const;
		VkFramebuffer getFramebuffer() const;
		const Size<uint32_t>& getSize() const;
		//
	public:
		//
		static VkRenderPass RequestRenderPassObject( ContextVk* _context, const RenderPassDescription& _desc );
	};

	class RenderPassSwapchainVk : public IRenderPass {
	private:
		ContextVk* m_context;
		VkRenderPass m_renderPass;
		std::vector< VkImage > m_vecImages;
		std::vector< TextureVk* > m_vecColors;
		//std::vector< VkImageView > m_vecImageViews;
		TextureVk* m_depthStencil;
		std::vector< VkFramebuffer> m_vecFramebuffers;
		Size<uint32_t> m_size;
		uint32_t m_imageIndex;
		//
		//uint8_t m_clearCount; // clear count
		VkClearValue m_clearValues[2]; // color & depth-stencil clear values
	public:
		RenderPassSwapchainVk() : m_renderPass( VK_NULL_HANDLE)
			, m_depthStencil(nullptr)
			, m_imageIndex(0)
		{
			memset(m_clearValues, 0, sizeof(m_clearValues));
			m_clearValues[1].depthStencil.depth = 1.0f;
		}
		//static IRenderPass* renderPassWithImages(std::vector<VkImage>& _images, uint32_t _width, uint32_t _height, VkFormat _format );
		void activeSubpass( uint32_t _imageIndex );
		//
		void update(std::vector<VkImage>& _images, uint32_t _width, uint32_t _height, VkFormat _format);
		void cleanup();
		//
		virtual bool begin() override;
		//
		virtual void resize(uint32_t _width, uint32_t _height) override;

		virtual void end() override;

		virtual void release() override
		{
			cleanup();
			delete this;
		}

		virtual void setClear(const RpClear& _clear) override;

		TextureVk* colorAttachment(uint32_t _index) {
			return m_vecColors[_index];
		}

	};
}