#pragma once
#include <NixRenderer.h>
#include <map>
#include "vkinc.h"
#include <vector>
#include <cassert>

namespace nix {

	class TextureVk;

	class NIX_API_DECL AttachmentVk:public IAttachment {
		friend class ContextVk;
	private:
		ContextVk*		m_context;
		TextureVk*		m_texture;
		Size<uint32_t>	m_size;
		NixFormat		m_format;
		VkBool32		m_isRef;
	public:
		//virtual void resize(uint32_t _width, uint32_t _height) override;
		virtual const ITexture* getTexture() const override;
		virtual NixFormat getFormat() const override;
		virtual void release() override;
		~AttachmentVk() {
			delete this;
		}
		static AttachmentVk* createAttachment( ContextVk* _context, TextureVk* _texture );
		static AttachmentVk* createAttachment( ContextVk* _context, NixFormat _format, uint32_t _width, uint32_t _height);
	};

	class NIX_API_DECL RenderPassVk : public IRenderPass {
		friend class ContextVk;
	private:
		ContextVk*					m_context;
		RenderPassDescription		m_desc;
		VkFramebuffer				m_framebuffer;
		VkRenderPass				m_renderPass;
		//
		AttachmentVk*				m_colorAttachments[MaxRenderTarget];
		AttachmentVk*				m_depthStencil;
		VkImageLayout				m_colorImageLayout[MaxRenderTarget];
		VkImageLayout				m_dsImageLayout;
		//
		uint8_t						m_clearCount; // clear count
		VkClearValue				m_clearValues[MaxRenderTarget+1]; // color & depth-stencil clear values
		//
		Size<uint32_t>				m_size;
		//
		VkCommandBuffer				m_commandBuffer;
		//
		static std::map< uint64_t, VkRenderPass > m_renderPassMapTable;
	public:
		RenderPassVk() {
			memset(m_colorAttachments, 0, sizeof(m_colorAttachments));
			memset(m_colorImageLayout, 0, sizeof(m_colorImageLayout));
			memset(m_clearValues, 0, sizeof(m_clearValues));
			m_clearValues[1].depthStencil.depth = 1.0f;
		}
		~RenderPassVk();
		//
		virtual bool begin() override;
		virtual void resize(uint32_t _width, uint32_t _height) override;
		virtual void end() override; 
		virtual void setClear( const RpClear& _clear ) override;
		
		virtual RenderPassType type() override {
			return OffscreenRenderPass;
		}
		virtual void bindPipeline(IPipeline* _pipeline) override;
		virtual void bindArgument(IArgument* _argument) override;
		//
		virtual void draw(IRenderable* _renderable, uint32_t _vertexOffset, uint32_t _vertexCount) override;
		virtual void drawElements(IRenderable* _renderable, uint32_t _indexOffset, uint32_t _indexCount) override;
		virtual void drawInstanced(IRenderable* _renderable, uint32_t _vertexOffset, uint32_t _vertexCount, uint32_t _baseInstance, uint32_t _instanceCount) override;
		virtual void drawElementInstanced(IRenderable* _renderable, uint32_t _indexOffset, uint32_t _indexCount, uint32_t _baseInstance, uint32_t _instanceCount) override;
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
		ContextVk*						m_context;
		VkRenderPass					m_renderPass;
		std::vector< VkImage >			m_vecImages;
		std::vector< TextureVk* >		m_vecColors;
		TextureVk*						m_depthStencil;
		std::vector< VkFramebuffer>		m_vecFramebuffers;
		Size<uint32_t>					m_size;
		uint32_t						m_imageIndex;
		VkCommandBuffer					m_commandBuffer;
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

		virtual RenderPassType type() override {
			return MainRenderPass;
		}

		TextureVk* colorAttachment(uint32_t _index) {
			return m_vecColors[_index];
		}

		virtual void bindPipeline(IPipeline* _pipeline) override;
		virtual void bindArgument(IArgument* _argument) override;
		//
		virtual void draw(IRenderable* _renderable, uint32_t _vertexOffset, uint32_t _vertexCount) override;
		virtual void drawElements(IRenderable* _renderable, uint32_t _indexOffset, uint32_t _indexCount) override;
		virtual void drawInstanced(IRenderable* _renderable, uint32_t _vertexOffset, uint32_t _vertexCount, uint32_t _baseInstance, uint32_t _instanceCount) override;
		virtual void drawElementInstanced(IRenderable* _renderable, uint32_t _indexOffset, uint32_t _indexCount, uint32_t _baseInstance, uint32_t _instanceCount) override;
	};
}