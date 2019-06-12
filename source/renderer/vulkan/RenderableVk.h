#pragma once
#include "VkInc.h"
#include <NixRenderer.h>

namespace Nix {

	class BufferVk;
	class ContextVk;
	class MaterialVk;

	class RenderableVk : public IRenderable {
		friend class Material;
	private:
		std::vector< VkBuffer >			m_vecBuffer;
		std::vector< VkDeviceSize >		m_vecBufferOffset;
		VkBuffer						m_indexBuffer;
		VkDeviceSize					m_indexBufferOffset;
		TopologyMode					m_topologyMode; // will no use, only for debugging
		//
		MaterialVk*						m_material;
		ContextVk*						m_context;
	public:

		RenderableVk(MaterialVk* _material);

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
		void draw(VkCommandBuffer _commandBuffer, uint32_t _vertexOffset, uint32_t _vertexCount);
		void drawElements(VkCommandBuffer _commandBuffer, uint32_t _indexOffset, uint32_t _indexCount);
		void drawInstanced(VkCommandBuffer _commandBuffer, uint32_t _vertexOffset, uint32_t _vertexCount, uint32_t _baseInstance, uint32_t _instanceCount);
		void drawElementInstanced(VkCommandBuffer _commandBuffer, uint32_t _indexOffset, uint32_t _indexCount, uint32_t _baseInstance, uint32_t _instanceCount);
		//
		virtual void release() override;

	};

}