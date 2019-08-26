#include "NixUIMeshBuffer.h"
#include "NixUIDefine.h"

namespace Nix {

	void UIMeshBuffer::initialize(IContext* _context, IRenderable* _renderable, uint32_t _vertexCount) {
		m_vertexBufferPM = _context->createVertexBufferPM(_vertexCount * sizeof(UIVertex), nullptr);
		m_indexBufferPM = _context->createIndexBufferPM(_vertexCount * sizeof(uint16_t), nullptr);
		//
		m_vertexBufferMemory.resize(_vertexCount);
		m_indexBufferMemory.resize(_vertexCount);
		//
		m_vertexCount = 0;
		m_indexCount = 0;
		//
		m_renderable = _renderable;
		m_renderable->setVertexBuffer(m_vertexBufferPM, 0, 0);
		m_renderable->setIndexBuffer(m_indexBufferPM, 0);
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
		}
		// can merge the draw call
		if ( _drawState == _lastState ) {
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
		m_vertexBufferPM->setData(m_vertexBufferMemory.data(), m_vertexCount * sizeof(UIVertex), 0);
		m_indexBufferPM->setData(m_indexBufferMemory.data(), m_indexCount * sizeof(uint16_t), 0);
	}
	 
	void UIMeshBuffer::draw(IRenderPass* _renderPass, IArgument* _argument, IPipeline* _pipeline, float _screenWidth, float _screenHeight) {
		struct Constants {
			float screenWidth;
			float screenHeight;
		} constants;

		constants.screenWidth = _screenWidth - 1;
		constants.screenHeight = _screenHeight - 1;

		for (auto& dc : this->m_vecCommands) {
			_renderPass->setScissor(dc.state.scissor);
			_argument->setShaderCache(0, &constants, sizeof(constants));
			_renderPass->bindArgument(_argument);
			_renderPass->drawElements(m_renderable, dc.indexOffset, dc.elementCount);
		}
	}

}