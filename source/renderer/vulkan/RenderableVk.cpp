#include "RenderableVk.h"
#include "RenderPassVk.h"
#include "MaterialVk.h"
#include "BufferVk.h"

namespace Nix {

	IRenderable* MaterialVk::createRenderable()
	{
		RenderableVk* renderable = new RenderableVk(this);
		return renderable;
	}

	void MaterialVk::destroyRenderable(IRenderable* _renderable)
	{
		RenderableVk* renderable = (RenderableVk*)_renderable;
		delete renderable;
	}

	RenderableVk::RenderableVk(MaterialVk* _material)
		:m_material(_material)
		,m_context(_material->getContext())
	{
		m_vecBuffer.resize(_material->getDescription().vertexLayout.bufferCount);
		m_vecBufferOffset.resize( m_vecBuffer.size() );
	}

	uint32_t RenderableVk::getVertexBufferCount()
	{
		return m_vecBuffer.size();
	}

	Nix::TopologyMode RenderableVk::getTopologyMode()
	{
		return m_topologyMode;
	}

	void RenderableVk::setVertexBuffer(IBuffer* _buffer, size_t _offset, uint32_t _index)
	{
		VkBuffer buffer = VK_NULL_HANDLE;
		VkDeviceSize offset = 0;
		auto type = _buffer->getType();
		switch (type)
		{
		case Nix::SVBO: {
			VertexBuffer* vertexBuffer = (VertexBuffer*)_buffer;
			buffer = *vertexBuffer;
			offset = vertexBuffer->getOffset();
			break;
		}
		case Nix::CVBO: {
			CachedVertexBuffer* vertexBuffer = (CachedVertexBuffer*)_buffer;
			buffer = *vertexBuffer;
			offset = vertexBuffer->getOffset();
			break;
		}
		case Nix::IBO:
		case Nix::UBO:
		case Nix::SSBO:
		case Nix::TBO:
		default:
			assert(false);
			break;
		}
		m_vecBuffer[_index] = buffer;
		m_vecBufferOffset[_index] = offset;
	}

	void RenderableVk::setIndexBuffer(IBuffer* _buffer, size_t _offset )
	{
		VkBuffer buffer = VK_NULL_HANDLE;
		VkDeviceSize offset = 0;
		auto type = _buffer->getType();
		switch (type)
		{
		case Nix::IBO: {
			IndexBuffer* indexBuffer = (IndexBuffer*)_buffer;
			buffer = *indexBuffer;
			offset = indexBuffer->getOffset();
			break;
		}
		case Nix::SVBO:
		case Nix::CVBO:
		case Nix::UBO:
		case Nix::SSBO:
		case Nix::TBO:
		default:
			assert(false);
			break;
		}
		m_indexBuffer = buffer;
		m_indexBufferOffset = offset;
	}

	void RenderableVk::release()
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void RenderableVk::draw(VkCommandBuffer _commandBuffer, uint32_t _vertexOffset, uint32_t _vertexCount)
	{
		vkCmdBindVertexBuffers(_commandBuffer, 0, m_vecBuffer.size(), m_vecBuffer.data(), m_vecBufferOffset.data());
		vkCmdDraw(_commandBuffer, _vertexCount, 1, _vertexOffset, 0);
	}

	void RenderableVk::drawElements(VkCommandBuffer _commandBuffer, uint32_t _indexOffset, uint32_t _indexCount)
	{
		if (m_vecBuffer.size()) {
			vkCmdBindVertexBuffers(_commandBuffer, 0, m_vecBuffer.size(), m_vecBuffer.data(), m_vecBufferOffset.data());
		}
		vkCmdBindIndexBuffer(_commandBuffer, m_indexBuffer, m_indexBufferOffset, VK_INDEX_TYPE_UINT16);
		vkCmdDrawIndexed(_commandBuffer, _indexCount, 1, _indexOffset, 0, 0);
	}

	void RenderableVk::drawInstanced(VkCommandBuffer _commandBuffer, uint32_t _vertexOffset, uint32_t _vertexCount, uint32_t _baseInstance, uint32_t _instanceCount)
	{
		vkCmdBindVertexBuffers(_commandBuffer, 0, m_vecBuffer.size(), m_vecBuffer.data(), m_vecBufferOffset.data());
		vkCmdDraw(_commandBuffer, _vertexCount, _instanceCount, _vertexOffset, _baseInstance);
	}

	void RenderableVk::drawElementInstanced(VkCommandBuffer _commandBuffer, uint32_t _indexOffset, uint32_t _indexCount, uint32_t _baseInstance, uint32_t _instanceCount)
	{
		if (m_vecBuffer.size()) {
			vkCmdBindVertexBuffers(_commandBuffer, 0, m_vecBuffer.size(), m_vecBuffer.data(), m_vecBufferOffset.data());
		}
		vkCmdBindIndexBuffer(_commandBuffer, m_indexBuffer, m_indexBufferOffset, VK_INDEX_TYPE_UINT16);
		vkCmdDrawIndexed(_commandBuffer, _indexCount, _instanceCount, _indexOffset, 0, _baseInstance);
	}

}