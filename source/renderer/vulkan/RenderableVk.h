#pragma once
#include "vkinc.h"
#include <NixRenderer.h>

namespace nix {

	class BufferVk;
	class ContextVk;

	class RenderableVk : public IRenderable {
	private:
		std::vector< VkBuffer >			m_vecBuffer;
		std::vector< VkDeviceSize >		m_vecBufferOffset;
		VkBuffer						m_indexBuffer;
		VkDeviceSize					m_indexBufferOffset;
		TopologyMode					m_topologyMode; // will no use, only for debugging
		ContextVk*						m_context;
	public:
		inline size_t getBufferCount() {
			return m_vecBuffer.size();
		}
		inline VkBuffer* getBuffers() {
			return m_vecBuffer.data();
		}
		inline VkDeviceSize* getBufferOffsets() {
			return m_vecBufferOffset.data();
		}
		inline VkBuffer getIndexBuffer() {
			return m_indexBuffer;
		}
		inline VkDeviceSize getIndexBufferOffset() {
			return m_indexBufferOffset;
		}
		//
		virtual uint32_t getVertexBufferCount() override;
		virtual TopologyMode getTopologyMode() override;
		// ------------------------------------------------------------------------
		//     setup resource binding
		// ------------------------------------------------------------------------
		virtual void setVertexBuffer(IBuffer* _buffer, size_t _offset, uint32_t _index) override;
		virtual void setIndexBuffer(IBuffer* _buffer, size_t _offset ) override;
		// ------------------------------------------------------------------------
		//     drawing
		// ------------------------------------------------------------------------
		void RenderableVk::draw(VkCommandBuffer _commandBuffer, uint32_t _vertexOffset, uint32_t _vertexCount);
		void RenderableVk::drawElements(VkCommandBuffer _commandBuffer, uint32_t _indexOffset, uint32_t _indexCount);
		void RenderableVk::drawInstanced(VkCommandBuffer _commandBuffer, uint32_t _vertexOffset, uint32_t _vertexCount, uint32_t _baseInstance, uint32_t _instanceCount);
		void RenderableVk::drawElementInstanced(VkCommandBuffer _commandBuffer, uint32_t _indexOffset, uint32_t _indexCount, uint32_t _baseInstance, uint32_t _instanceCount);
		//
		virtual void release() override;

	};

}