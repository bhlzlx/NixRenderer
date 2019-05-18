#pragma once
#include "vkinc.h"

namespace nix {

	class BufferVk;

	class RenderableVk {
	private:
		std::vector< VkBuffer >			m_vecBuffer;
		std::vector< VkDeviceSize >		m_vecBufferOffset;
		VkBuffer						m_indexBuffer;
		VkDeviceSize					m_indexBufferOffset;
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
			return m_indexBufferOffset
		}
		//
		inline void setVertexBuffer(uint32_t _binding, VkBuffer _buffer, VkDeviceSize _offset) {
			m_vecBuffer[_binding] = _buffer;
			m_vecBufferOffset[_binding] = _offset;
		}
		inline void setIndexBuffer(VkBuffer _buffer, VkDeviceSize _offset) {
			m_indexBuffer = _buffer;
			m_indexBufferOffset = _offset;
		}
	};

}