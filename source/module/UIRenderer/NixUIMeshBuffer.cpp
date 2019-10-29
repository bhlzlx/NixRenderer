#include "NixUIMeshBuffer.h"
#include "NixUIDefine.h"

namespace Nix {

	void UIMeshBuffer::initialize(IContext* _context, IRenderable* _renderable, uint32_t _rectCount) {
		m_vertexBuffer = _context->createVertexBuffer(nullptr, _rectCount * 4 * sizeof(UIVertex));
		m_indexBuffer = _context->createIndexBuffer(nullptr, _rectCount * 6 * sizeof(uint16_t));
		//
		m_vertexBufferMemory.resize(_rectCount * 4 * sizeof(UIVertex));
		m_indexBufferMemory.resize(_rectCount * 6 * sizeof(uint16_t));
		m_uniformBufferMemory.resize(_rectCount);
		//
		m_vertexCount = 0;
		m_indexCount = 0;
		//
		m_renderable = _renderable;
		m_renderable->setVertexBuffer(m_vertexBuffer, 0, 0);
		m_renderable->setIndexBuffer(m_indexBuffer, 0);
	}

	bool UIMeshBuffer::pushVertices(const UIDrawData* _drawData, const UIDrawState& _drawState, const UIDrawState& _lastState) {

		uint32_t dcVtxCount = 0;
		uint32_t dcIdxCount = 0;

		if (_drawData->type == UITopologyType::UIRectangle) {
			dcVtxCount = _drawData->primitiveCount * 4;
			dcIdxCount = _drawData->primitiveCount * 4 * 3 / 2;
			//
			if (dcVtxCount + m_vertexCount > m_indexBufferMemory.size()) {
				return false;
			}
			uint16_t baseIndex = m_vertexCount;
			// copy vertices data
			UIVertex* begin = (UIVertex*)_drawData->vertexBufferAllocation.ptr;
			UIVertex* end = begin + dcVtxCount;
			memcpy(&m_vertexBufferMemory[m_vertexCount], begin, dcVtxCount * sizeof(UIVertex));
			m_vertexCount += dcVtxCount;
			// copy indices data
			for (uint32_t i = 0; i < _drawData->primitiveCount; ++i) {
				uint16_t rcIndices[6] = {
					baseIndex, baseIndex + 1, baseIndex + 2,
					baseIndex + 2, baseIndex + 3, baseIndex
				};
				memcpy(&m_indexBufferMemory[m_indexCount], rcIndices, sizeof(rcIndices));
				baseIndex += 4;
				m_indexCount += 6;
			}
			//
			memcpy(&m_uniformBufferMemory[m_uniformCount], _drawData->uniformData.data(), _drawData->uniformData.size() * sizeof(UIUniformElement));
			m_uniformCount += _drawData->uniformData.size();
		}
		else {
			dcVtxCount = _drawData->primitiveCount * 3;
			dcIdxCount = _drawData->primitiveCount * 3;
			//
			if (dcVtxCount + m_vertexCount > m_indexBufferMemory.size()) {
				return false;
			}
			uint16_t baseIndex = m_vertexCount;
			// copy vertices data
			UIVertex* begin = (UIVertex*)_drawData->vertexBufferAllocation.ptr;
			UIVertex* end = begin + dcVtxCount;
			memcpy(&m_vertexBufferMemory[m_vertexCount], begin, dcVtxCount * sizeof(UIVertex));
			m_vertexCount += dcVtxCount;
			// copy indices data
			for (uint32_t i = 0; i < _drawData->primitiveCount; ++i) {
				uint16_t rcIndices[3] = {
					baseIndex, baseIndex + 1, baseIndex + 2
				};
				memcpy(&m_indexBufferMemory[m_indexCount], rcIndices, sizeof(rcIndices));
				baseIndex += 3;
				m_indexCount += 3;
			}
			memcpy(&m_uniformBufferMemory[m_uniformCount], _drawData->uniformData.data(), _drawData->uniformData.size() * sizeof(UIUniformElement));
			m_uniformCount += _drawData->uniformData.size();
		}
		// can merge the draw call
		if (_drawState == _lastState) {
			UIDrawBatch& cmd = m_vecCommands.back();
			cmd.elementCount += dcIdxCount;
		}
		else
		{
			UIDrawBatch cmd;
			cmd.elementCount = dcIdxCount;
			cmd.indexOffset = (m_indexCount - dcIdxCount);
			cmd.vertexOffset = (m_vertexCount - dcVtxCount) * sizeof(UIVertex);
			cmd.state = _drawState;
			m_vecCommands.push_back(cmd);
		}
		return true;
	}

	void UIMeshBuffer::flushMeshBuffer() {
		m_vertexBuffer->updateData(m_vertexBufferMemory.data(), m_vertexCount * sizeof(UIVertex), 0);
		m_indexBuffer->updateData(m_indexBufferMemory.data(), m_indexCount * sizeof(uint16_t), 0);
	}

	void UIMeshBuffer::draw(IRenderPass* _renderPass, IArgument* _argument, uint32_t& _rectCount) {
		for (auto& dc : this->m_vecCommands) {
			uint32_t uniformOffset = _rectCount;
			_argument->setShaderCache(8, &uniformOffset, 4);
			_renderPass->setScissor(dc.state.scissor);
			_renderPass->drawElements(m_renderable, dc.indexOffset, dc.elementCount);
			_rectCount += dc.elementCount / 6;
		}
	}

}
