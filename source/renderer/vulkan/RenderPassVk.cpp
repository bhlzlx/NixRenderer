#include <NixRenderer.h>
#include "RenderPassVk.h"
#include "TextureVk.h"
#include "ContextVk.h"
#include "BufferVk.h"
#include "QueueVk.h"
#include "PipelineVk.h"
#include "vkhelper/helper.h"
#include "TypemappingVk.h"
#include "RenderableVk.h"
#include "nix/string/encoding.h"
#include <cassert>

namespace Nix {
	//
	std::map< uint64_t, VkRenderPass > RenderPassVk::m_standardMapTable;
	std::map< uint64_t, VkRenderPass > RenderPassVk::m_compatMapTable;
	//
	VkImageLayout AttachmentUsageToImageLayout(AttachmentUsageBits _usage);
	//

	uint64_t RenderPassCompatibleHash(const RenderPassDescription& _desc) {
		APHasher hasher;
		if (_desc.colorCount) {
			for (uint32_t i = 0; i < _desc.colorCount; ++i) {
				hasher.hash(&_desc.colors[i].format, sizeof(_desc.colors[i].format));
				hasher.hash(&_desc.colors[i].multisample, sizeof(_desc.colors[i].multisample));
			}
		}
		if (_desc.depthStencil.format != NixInvalidFormat) {
			hasher.hash(&_desc.depthStencil.format, sizeof(_desc.depthStencil.format));
			hasher.hash(&_desc.depthStencil.multisample, sizeof(_desc.depthStencil.multisample));
		}
		if (_desc.inputCount) {
			for (uint32_t i = 0; i < _desc.inputCount; ++i) {
				hasher.hash(&_desc.inputs[i].format, sizeof(_desc.inputs[i].format));
				hasher.hash(&_desc.inputs[i].multisample, sizeof(_desc.inputs[i].multisample));
			}
		}
		if (_desc.resolve.format != NixInvalidFormat) {
			hasher.hash(&_desc.resolve.format, sizeof(_desc.resolve.format));
			hasher.hash(&_desc.resolve.multisample, sizeof(_desc.resolve.multisample));
		}
		return hasher;
	}

	uint64_t RenderPassStandardHash(const RenderPassDescription& _desc) {
		APHasher hasher;
		if (_desc.colorCount) {
			for (uint32_t i = 0; i < _desc.colorCount; ++i ) {
				hasher.hash(&_desc.colors[i].format, sizeof(_desc.colors[i].format));
				hasher.hash(&_desc.colors[i].loadAction, sizeof(_desc.colors[i].loadAction));
				hasher.hash(&_desc.colors[i].usage, sizeof(_desc.colors[i].usage));
				hasher.hash(&_desc.colors[i].multisample, sizeof(_desc.colors[i].multisample));
			}
		}
		if (_desc.depthStencil.format != NixInvalidFormat) {
			hasher.hash(&_desc.depthStencil.format, sizeof(_desc.depthStencil.format));
			hasher.hash(&_desc.depthStencil.loadAction, sizeof(_desc.depthStencil.loadAction));
			hasher.hash(&_desc.depthStencil.usage, sizeof(_desc.depthStencil.usage));
			hasher.hash(&_desc.depthStencil.multisample, sizeof(_desc.depthStencil.multisample));
		}
		if (_desc.inputCount) {
			for (uint32_t i = 0; i < _desc.inputCount; ++i) {
				hasher.hash(&_desc.inputs[i].format, sizeof(_desc.inputs[i].format));
				hasher.hash(&_desc.inputs[i].loadAction, sizeof(_desc.inputs[i].loadAction));
				hasher.hash(&_desc.inputs[i].usage, sizeof(_desc.inputs[i].usage));
				hasher.hash(&_desc.inputs[i].multisample, sizeof(_desc.inputs[i].multisample));
			}
		}
		if (_desc.resolve.format != NixInvalidFormat) {
			hasher.hash(&_desc.resolve.format, sizeof(_desc.resolve.format));
			hasher.hash(&_desc.resolve.loadAction, sizeof(_desc.resolve.loadAction));
			hasher.hash(&_desc.resolve.usage, sizeof(_desc.resolve.usage));
			hasher.hash(&_desc.resolve.multisample, sizeof(_desc.resolve.multisample));
		}
		return hasher;
	}

	AttachmentVk* AttachmentVk::createAttachment(ContextVk* _context, TextureVk* _texture) {
		AttachmentVk* attachment = new AttachmentVk();
		attachment->m_isRef = VK_TRUE;
		attachment->m_size = {
			_texture->getDesc().width, _texture->getDesc().height
		};
		attachment->m_format = _texture->getDesc().format;
		attachment->m_texture = _texture;
		attachment->m_context = _context;
		return attachment;
	}

	AttachmentVk* AttachmentVk::createAttachment(ContextVk* _context, NixFormat _format, uint32_t _width, uint32_t _height) {
		AttachmentVk* attachment = new AttachmentVk();
		attachment->m_isRef = VK_FALSE;
		attachment->m_size = { _width, _height };
		attachment->m_format = _format;

		TextureDescription desc; {
			desc.type = Texture2D;
			desc.format = _format;
			desc.mipmapLevel = 1;
			desc.height = _height;
			desc.width = _width;
			desc.depth = 1;
		}
		auto format = NixFormatToVk(_format);
		TextureUsageFlags usage =
			TextureUsageFlagBits::TextureUsageTransferDestination |
			TextureUsageFlagBits::TextureUsageTransferSource |
			TextureUsageFlagBits::TextureUsageSampled;
		if (vkhelper::isDepthFormat(format) || vkhelper::isStencilFormat(format)) {
			usage |= TextureUsageFlagBits::TextureUsageDepthStencilAttachment;
//			usage = TextureUsageDepthStencilAttachment;
			
		}
		else {
			usage |= TextureUsageFlagBits::TextureUsageColorAttachment;
		}
		//
		attachment->m_texture = TextureVk::createTexture(_context, VK_NULL_HANDLE, VK_NULL_HANDLE, desc, usage);
		attachment->m_context = _context;
		//
		return attachment;
	}

	IAttachment* ContextVk::createAttachment( NixFormat _format, uint32_t _width, uint32_t _height) {
		return AttachmentVk::createAttachment( this, _format, _width, _height);
	}

	IRenderPass* ContextVk::createRenderPass(const RenderPassDescription& _desc, IAttachment** _colorAttachments, IAttachment* _depthStencil) {
		RenderPassVk* renderPass = new RenderPassVk();
		renderPass->m_desc = _desc;
		uint32_t clearCount = 0;
		for (uint32_t i = 0; i < _desc.colorCount; ++i) {
			renderPass->m_colorAttachments[i] = dynamic_cast<AttachmentVk*>(_colorAttachments[i]);
			if (_desc.colors[i].loadAction == Clear) {
				++clearCount;
			}
		}
		if (_desc.depthStencil.format != NixInvalidFormat) {
			++clearCount;
			assert(_depthStencil && "must not be nullptr!!!");
			renderPass->m_depthStencil = dynamic_cast<AttachmentVk*>(_depthStencil);
		}
		renderPass->m_renderPass = RenderPassVk::RequestStandardRenderPassObject( this,_desc );
		renderPass->m_clearCount = clearCount;
		for (uint32_t i = 0; i < _desc.colorCount; ++i) {
			renderPass->m_colorImageLayout[i] = AttachmentUsageToImageLayout(_desc.colors[i].usage);
		}
		renderPass->m_dsImageLayout = AttachmentUsageToImageLayout(_desc.depthStencil.usage);
		renderPass->m_framebuffer = VK_NULL_HANDLE;
		//
		{
			uint32_t attachmentCount = _desc.colorCount;
			VkImageView imageViews[MaxRenderTarget + 1];
			for (uint32_t i = 0; i<_desc.colorCount; ++i)
			{
				renderPass->m_colorAttachments[i] = (AttachmentVk*)_colorAttachments[i];
				imageViews[i] = ((TextureVk*)renderPass->m_colorAttachments[i]->getTexture())->getImageView();
			}
			if (_depthStencil) {
				renderPass->m_depthStencil = (AttachmentVk*)_depthStencil;
				imageViews[attachmentCount] = ((TextureVk*)_depthStencil->getTexture())->getImageView();
				++attachmentCount;
			}
			//
			AttachmentVk* availAttachmentRef = (AttachmentVk*) (_desc.colorCount ? _colorAttachments[0] : _depthStencil);
			renderPass->m_size = {
				availAttachmentRef->getTexture()->getDesc().width, availAttachmentRef->getTexture()->getDesc().height
			};
			VkFramebufferCreateInfo fbinfo = {}; {
				fbinfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
				fbinfo.pNext = nullptr;
				fbinfo.flags = 0;
				fbinfo.attachmentCount = attachmentCount;
				fbinfo.pAttachments = imageViews;
				fbinfo.width = renderPass->m_size.width;
				fbinfo.height = renderPass->m_size.height;
				fbinfo.layers = 1;
				fbinfo.renderPass = renderPass->m_renderPass;
			}
			if (VK_SUCCESS != vkCreateFramebuffer( getDevice(), &fbinfo, nullptr, &renderPass->m_framebuffer))
			{
				delete renderPass;
				return nullptr;
			}
		}
		renderPass->m_context = this;
		//
		return renderPass;
	}

	const ITexture* AttachmentVk::getTexture() const {
		return m_texture;
	}

	void Nix::AttachmentVk::release() {
		if (!m_isRef) {
			m_texture->release();
		}
	}

	Nix::NixFormat Nix::AttachmentVk::getFormat() const {
		return m_format;
	}

	RenderPassVk::~RenderPassVk()
	{
		// VkRenderPass obejct was managered by a std::map
		// so we don't need to destroy the VkRenderPass object
		// attachment was not managed by the  render pass 
		// so we don't destroy it neither
		vkDestroyFramebuffer( m_context->getDevice(), m_framebuffer, nullptr);
		delete this;
	}

	bool RenderPassVk::begin( IGraphicsQueue* _queue )
	{
		GraphicsQueueVk* queue = (GraphicsQueueVk*)_queue;
		auto cmdbuff = queue->commandBuffer();
		//m_commandBuffer = (const VkCommandBuffer&)cmdbuff;
		m_commandBuffer = (const VkCommandBuffer&)*cmdbuff;
		// transform attachment image layout
		for (auto& attachment : m_colorAttachments) {
			if (!attachment)
				break;
			TextureVk* texture = (TextureVk*)attachment->getTexture();
			texture->transformImageLayout( m_commandBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		}
		TextureVk* texture = (TextureVk*)m_depthStencil->getTexture();
		if (texture) {
			texture->transformImageLayout( m_commandBuffer, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
		}
		// create begin info
		VkRenderPassBeginInfo beginInfo = {
			VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,// VkStructureType sType;
			nullptr,// const void* pNext;
			m_renderPass,// VkRenderPass renderPass;
			m_framebuffer,// VkFramebuffer framebuffer;
			{
				0, 0, m_size.width, m_size.height
			},// VkRect2D renderArea;
			m_clearCount,
			m_clearValues// const VkClearValue* pClearValues;
		};
		//
		vkCmdBeginRenderPass( m_commandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
		return true;
	}

	void RenderPassVk::resize(uint32_t _width, uint32_t _height)
	{

	}

	void RenderPassVk::end()
	{
		vkCmdEndRenderPass(m_commandBuffer);
		m_commandBuffer = VK_NULL_HANDLE;
		//
		for ( uint32_t i = 0; i<m_desc.colorCount; ++i )
		{
			TextureVk* texture = (TextureVk*)m_colorAttachments[i]->getTexture();
			texture->setImageLayout(m_colorImageLayout[i]);
		}
		if (m_depthStencil) {
			TextureVk* texture = (TextureVk*)m_depthStencil->getTexture();
			texture->setImageLayout(m_dsImageLayout);
		}
	}

	void RenderPassVk::setClear(const RpClear& _clear)
	{
		uint32_t clearCount = 0;
		for (uint32_t i = 0; i < m_desc.colorCount; ++i) {
			if (m_desc.colors->loadAction == Clear) {
				m_clearValues[clearCount].color.float32[0] = _clear.colors[i].r;
				m_clearValues[clearCount].color.float32[1] = _clear.colors[i].g;
				m_clearValues[clearCount].color.float32[2] = _clear.colors[i].b;
				m_clearValues[clearCount].color.float32[3] = _clear.colors[i].a;
				++clearCount;
			}
		}
		m_clearValues[clearCount].depthStencil.depth = _clear.depth;
		m_clearValues[clearCount].depthStencil.stencil = _clear.stencil;
	}

	void RenderPassVk::bindPipeline(IPipeline* _pipeline)
	{
		PipelineVk* pipeline = (PipelineVk*)_pipeline;
		vkCmdBindPipeline( m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipeline );
		pipeline->setDynamicalStates(m_commandBuffer);
	}

	void RenderPassVk::bindArgument(IArgument* _argument)
	{
		ArgumentVk* argument = (ArgumentVk*)_argument;
		argument->bind(m_commandBuffer);
	}

	void RenderPassVk::draw(IRenderable* _renderable, uint32_t _vertexOffset, uint32_t _vertexCount)
	{
		RenderableVk* renderable = (RenderableVk*)_renderable;
		renderable->draw(m_commandBuffer, _vertexOffset, _vertexCount);
	}

	void RenderPassVk::drawElements(IRenderable* _renderable, uint32_t _indexOffset, uint32_t _indexCount)
	{
		RenderableVk* renderable = (RenderableVk*)_renderable;
		renderable->drawElements(m_commandBuffer, _indexOffset, _indexCount);
	}

	void RenderPassVk::drawInstanced(IRenderable* _renderable, uint32_t _vertexOffset, uint32_t _vertexCount, uint32_t _baseInstance, uint32_t _instanceCount)
	{
		RenderableVk* renderable = (RenderableVk*)_renderable;
		renderable->drawInstanced(m_commandBuffer, _vertexOffset, _vertexCount, _baseInstance, _instanceCount);
	}

	void RenderPassVk::drawElementInstanced(IRenderable* _renderable, uint32_t _indexOffset, uint32_t _indexCount, uint32_t _baseInstance, uint32_t _instanceCount)
	{
		RenderableVk* renderable = (RenderableVk*)_renderable;
		renderable->drawElementInstanced(m_commandBuffer, _indexOffset, _indexCount, _baseInstance, _instanceCount);
	}

	const Nix::RenderPassDescription& RenderPassVk::getDescption() const
	{
		return m_desc;
	}

	VkFramebuffer RenderPassVk::getFramebuffer() const
	{
		return m_framebuffer;
	}

	const Nix::Size<uint32_t>& RenderPassVk::getSize() const
	{
		return m_size;
	}

	VkAttachmentLoadOp RTLoadOpToVk(Nix::AttachmentLoadAction _load) {
		switch (_load) {
		case Clear:
			return VK_ATTACHMENT_LOAD_OP_CLEAR;
		case Keep:
			return VK_ATTACHMENT_LOAD_OP_LOAD;
		case DontCare:
			return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		}
		return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	}

	VkImageLayout AttachmentUsageToImageLayout(AttachmentUsageBits _usage) {
		switch (_usage) {
		case AOU_ShaderRead: return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		case AOU_DepthStencilAttachment: return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		case AOU_ColorAttachment: return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		case AOU_Present: return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		}
		return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	}

	VkRenderPass RenderPassVk::RequestStandardRenderPassObject(ContextVk* _context, const RenderPassDescription& _desc)
	{
		uint64_t stdHash = RenderPassStandardHash(_desc);
		uint64_t compatHash = RenderPassCompatibleHash(_desc);
		//
		auto it = m_standardMapTable.find(stdHash);
		if (it != m_standardMapTable.end()) {
			return it->second;
		}
		//
		auto sampleCount = VK_SAMPLE_COUNT_1_BIT;
		std::vector<VkAttachmentDescription> attacmentDescriptions; {
			for (uint32_t i = 0; i < _desc.colorCount; ++i) {
				VkAttachmentDescription desc; {
					desc.flags = 0;
					desc.format = NixFormatToVk(_desc.colors[i].format);
					desc.samples = sampleCount;
					desc.loadOp = RTLoadOpToVk(_desc.colors[i].loadAction);
					desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // does not support tiled rendering
					desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // color attachment dose not contains this
					desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
					desc.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
					desc.finalLayout = AttachmentUsageToImageLayout(_desc.colors[i].usage);
				}
				attacmentDescriptions.push_back(desc);
			}
			{
				VkAttachmentDescription desc; {
					desc.flags = 0;
					desc.format = NixFormatToVk(_desc.depthStencil.format);
					desc.samples = sampleCount;
					desc.loadOp = RTLoadOpToVk(_desc.depthStencil.loadAction);
					desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
					desc.stencilLoadOp = RTLoadOpToVk(_desc.depthStencil.loadAction);
					desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
					desc.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
					desc.finalLayout = AttachmentUsageToImageLayout(_desc.depthStencil.usage);;
				}
				attacmentDescriptions.push_back(desc);
			}
		}
		// setup color attachment reference -> attachment descriptions
		std::vector<VkAttachmentReference> colorAttachments; {
			VkAttachmentReference color;
			color.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			for (uint32_t i = 0; i < _desc.colorCount; ++i) {
				color.attachment = i;
				colorAttachments.push_back(color);
			}
		}
		// setup depthstencil attachment reference -> attachment description
		VkAttachmentReference depthstencil = {
			_desc.colorCount, // attachment
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL // layout
		};
		// subpass description
		VkSubpassDescription subpassinfo; {
			subpassinfo.flags = 0;
			subpassinfo.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpassinfo.pInputAttachments = nullptr;
			subpassinfo.inputAttachmentCount = 0;
			subpassinfo.pPreserveAttachments = nullptr;
			subpassinfo.preserveAttachmentCount = 0;
			subpassinfo.pResolveAttachments = nullptr;
			// color attachments
			subpassinfo.colorAttachmentCount = static_cast<uint32_t>(_desc.colorCount);
			if (subpassinfo.colorAttachmentCount)
				subpassinfo.pColorAttachments = &colorAttachments[0];
			// depth stencil
			//if (_pDS)
			subpassinfo.pDepthStencilAttachment = &depthstencil;
			//else
			//subpassinfo.pDepthStencilAttachment = nullptr;
		}
		// subpass dependency
		VkSubpassDependency subpassDependency[3]; {
			subpassDependency[0].srcSubpass = VK_SUBPASS_EXTERNAL;
			subpassDependency[0].dstSubpass = 0;
			subpassDependency[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			subpassDependency[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			subpassDependency[0].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
			subpassDependency[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			subpassDependency[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
			//
			subpassDependency[1].srcSubpass = 0;
			subpassDependency[1].dstSubpass = VK_SUBPASS_EXTERNAL;
			subpassDependency[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			subpassDependency[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			subpassDependency[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			subpassDependency[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
			subpassDependency[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
			//
			subpassDependency[2].srcSubpass = 0;
			subpassDependency[2].dstSubpass = 0;
			subpassDependency[2].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			subpassDependency[2].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			subpassDependency[2].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			subpassDependency[2].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
			subpassDependency[2].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
		}
		// render pass create info
		VkRenderPassCreateInfo rpinfo; {
			rpinfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			rpinfo.pNext = nullptr;
			rpinfo.flags = 0;
			rpinfo.pAttachments = &attacmentDescriptions[0];
			rpinfo.attachmentCount = static_cast<uint32_t>(attacmentDescriptions.size());
			rpinfo.subpassCount = 1;
			rpinfo.pSubpasses = &subpassinfo;
			rpinfo.dependencyCount = 3;
			rpinfo.pDependencies = &subpassDependency[0];
		}
		// create render pass object
		VkRenderPass renderpass;
		VkResult rst = vkCreateRenderPass( _context->getDevice(), &rpinfo, nullptr, &renderpass);
		if (rst != VK_SUCCESS) {
			return VK_NULL_HANDLE;
		}
		m_standardMapTable[stdHash] = renderpass;
		//
		if (m_compatMapTable.find(compatHash) == m_compatMapTable.end()) {
			m_compatMapTable[compatHash] = renderpass;
		}
		return renderpass;
	}

	VkRenderPass RenderPassVk::RequestCompatibleRenderPassObject(ContextVk* _context, const RenderPassDescription& _desc) {
		uint64_t compatHash = RenderPassStandardHash(_desc);
		if ( m_compatMapTable.find(compatHash) == m_compatMapTable.end() ) {
			return RequestStandardRenderPassObject(_context, _desc);
		}
	}

	void RenderPassSwapchainVk::activeSubpass(uint32_t _imageIndex)
	{
		m_imageIndex = _imageIndex;
	}

	void RenderPassSwapchainVk::update(std::vector<VkImage>& _images, uint32_t _width, uint32_t _height, VkFormat _format)
	{
		cleanup();
		// create image views
		VkImageViewCreateInfo ivinfo = {}; {
			ivinfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			ivinfo.image = VK_NULL_HANDLE;
			ivinfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			ivinfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			ivinfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			ivinfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			ivinfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			ivinfo.subresourceRange.baseMipLevel = 0;
			ivinfo.subresourceRange.levelCount = 1;
			ivinfo.subresourceRange.baseArrayLayer = 0;
			ivinfo.subresourceRange.layerCount = 1;
			ivinfo.pNext = nullptr;
			ivinfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			ivinfo.format = _format;
			ivinfo.flags = 0;
			// texture aspect should have all aspect information
			// but the image view should not have the stencil aspect
			ivinfo.subresourceRange.aspectMask &= ~VK_IMAGE_ASPECT_STENCIL_BIT;
		}
		auto device = m_context->getDevice();
		//
		TextureDescription dsdesc = {}; {
			dsdesc.width = _width;
			dsdesc.height = _height;
			dsdesc.depth = 1;
			dsdesc.format = NixDepth32F;
			dsdesc.mipmapLevel = 1;
			dsdesc.type = Texture2D;
		}
		m_depthStencil = (TextureVk*)TextureVk::createTexture( m_context, VK_NULL_HANDLE, VK_NULL_HANDLE, dsdesc, TextureUsageDepthStencilAttachment);
		//
		m_vecImages = std::move(_images);
		for (auto& image : m_vecImages) {
			dsdesc.format = VkFormatToNix(_format);
			auto color = TextureVk::createTexture( m_context, image, VK_NULL_HANDLE, dsdesc, TextureUsageColorAttachment );
			m_vecColors.push_back(color);
		}
		RenderPassDescription rpdesc = {}; {
			rpdesc.colors[0].format = VkFormatToNix(_format);
			rpdesc.colors[0].loadAction = Clear;
			rpdesc.colors[0].usage = AOU_Present;
			rpdesc.colorCount = 1;
			rpdesc.depthStencil.format = NixDepth32F;
			rpdesc.depthStencil.loadAction = Clear;
			rpdesc.depthStencil.usage = AOU_DepthStencilAttachment;
		}
		m_renderPass = RenderPassVk::RequestStandardRenderPassObject( m_context, rpdesc);
		//
		for (uint32_t i = 0; i < m_vecColors.size(); ++i) {
			VkImageView ivs[2] = {
				m_vecColors[i]->getImageView(),
				m_depthStencil->getImageView()
			};
			VkFramebufferCreateInfo fbinfo = {}; {
				fbinfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
				fbinfo.pNext = nullptr;
				fbinfo.flags = 0;
				fbinfo.attachmentCount = 2;
				fbinfo.pAttachments = ivs;
				fbinfo.width = _width;
				fbinfo.height = _height;
				fbinfo.layers = 1;
				fbinfo.renderPass = m_renderPass;
			}
			VkFramebuffer fbo = VK_NULL_HANDLE;
			auto rst = vkCreateFramebuffer(m_context->getDevice(), &fbinfo, nullptr, &fbo);
			assert(rst == VK_SUCCESS);
			m_vecFramebuffers.push_back(fbo);
		}
		m_size = {
			_width, _height
		};
	}

	void RenderPassSwapchainVk::cleanup()
	{
		for ( auto& fbo : m_vecFramebuffers ) {
			vkDestroyFramebuffer(m_context->getDevice(), fbo, nullptr);
		}
		for ( auto& iv : m_vecColors ) {
			delete iv;
		}
		m_vecColors.clear();
		m_vecFramebuffers.clear();
		if (m_depthStencil) {
			delete m_depthStencil;
			m_depthStencil = nullptr;
		}
	}

	bool RenderPassSwapchainVk::begin(IGraphicsQueue* _queue)
	{
		GraphicsQueueVk* queue = (GraphicsQueueVk*)_queue;
		auto cmdbuff = queue->commandBuffer();
		m_commandBuffer = *cmdbuff;
		// transform attachment image layout
		m_vecColors[m_imageIndex]->transformImageLayout(m_commandBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		m_depthStencil->transformImageLayout(m_commandBuffer, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
		// create begin info
		VkRenderPassBeginInfo beginInfo = {
			VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,// VkStructureType sType;
			nullptr,// const void* pNext;
			m_renderPass,// VkRenderPass renderPass;
			m_vecFramebuffers[m_imageIndex],// VkFramebuffer framebuffer;
			{
				0, 0, m_size.width, m_size.height
			},// VkRect2D renderArea;
			2,
			m_clearValues// const VkClearValue* pClearValues;
		};
		//
		vkCmdBeginRenderPass(m_commandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
		//
		return true;
	}

	void RenderPassSwapchainVk::resize(uint32_t _width, uint32_t _height)
	{
		return;
	}

	void RenderPassSwapchainVk::end()
	{
		vkCmdEndRenderPass(m_commandBuffer);
		m_commandBuffer = VK_NULL_HANDLE;
		//
		m_vecColors[m_imageIndex]->setImageLayout(VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
		m_depthStencil->setImageLayout(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	}

	void RenderPassSwapchainVk::setClear(const RpClear& _clear)
	{
		m_clearValues[0].color.float32[0] = _clear.colors[0].r;
		m_clearValues[0].color.float32[1] = _clear.colors[0].g;
		m_clearValues[0].color.float32[2] = _clear.colors[0].b;
		m_clearValues[0].color.float32[3] = _clear.colors[0].a;
		m_clearValues[1].depthStencil.depth = _clear.depth;
		m_clearValues[1].depthStencil.stencil = _clear.stencil;
	}

	void RenderPassSwapchainVk::bindPipeline(IPipeline* _pipeline)
	{
		PipelineVk* pipeline = (PipelineVk*)_pipeline;
		vkCmdBindPipeline(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipeline);
		pipeline->setDynamicalStates(m_commandBuffer);
	}

	void RenderPassSwapchainVk::bindArgument(IArgument* _argument)
	{
		ArgumentVk* argument = (ArgumentVk*)_argument;
		argument->bind(m_commandBuffer);
	}

	void RenderPassSwapchainVk::draw(IRenderable* _renderable, uint32_t _vertexOffset, uint32_t _vertexCount)
	{
		RenderableVk* renderable = (RenderableVk*)_renderable;
		renderable->draw(m_commandBuffer, _vertexOffset, _vertexCount);
	}

	void RenderPassSwapchainVk::drawElements(IRenderable* _renderable, uint32_t _indexOffset, uint32_t _indexCount)
	{
		RenderableVk* renderable = (RenderableVk*)_renderable;
		renderable->drawElements(m_commandBuffer, _indexOffset, _indexCount);
	}

	void RenderPassSwapchainVk::drawInstanced(IRenderable* _renderable, uint32_t _vertexOffset, uint32_t _vertexCount, uint32_t _baseInstance, uint32_t _instanceCount)
	{
		RenderableVk* renderable = (RenderableVk*)_renderable;
		renderable->drawInstanced(m_commandBuffer, _vertexOffset, _vertexCount, _baseInstance, _instanceCount);
	}

	void RenderPassSwapchainVk::drawElementInstanced(IRenderable* _renderable, uint32_t _indexOffset, uint32_t _indexCount, uint32_t _baseInstance, uint32_t _instanceCount)
	{
		RenderableVk* renderable = (RenderableVk*)_renderable;
		renderable->drawElementInstanced(m_commandBuffer, _indexOffset, _indexCount, _baseInstance, _instanceCount);
	}

}



