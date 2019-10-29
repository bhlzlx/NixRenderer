#pragma once
#include <NixRenderer.h>
#include <vector>
#include "NixUIDefine.h"

namespace Nix {

	/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
	*
	*  class `UIMeshBuffer` handles `vertex buffer` & `index buffer` for an
	*  `IRenderable`, each frame may refer to several `UIMeshBuffer` object and
	*  each `UIMeshBuffer` object contains `MaxVertexCount`(4096 * 4) vertices
	*
	*  when refer to the `draw call`, each `UIMeshBuffer` may contains vertices for
	*  several `draw call`, `draw calls` split by different `UIDrawState`
	*  if vertices in an `UIMeshBuffer` have the same `UIDrawState`, then, all vertices
	*  in this `UIMeshBuffer` object can be grouped into one `draw call`
	*
	* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

	struct UIDrawData;

	class UIMeshBuffer {
	private:
		IRenderable*						m_renderable;
		IBuffer*							m_vertexBuffer;
		std::vector<UIVertex>				m_vertexBufferMemory;
		IBuffer*							m_indexBuffer;
		std::vector<uint16_t>				m_indexBufferMemory;
		IBuffer*							m_uniformBuffer;
		std::vector<UIUniformElement>		m_uniformBufferMemory;
		//
		uint32_t							m_vertexCount;
		uint32_t							m_indexCount;
		uint32_t							m_uniformCount;// rect count
		//
		std::vector<UIDrawBatch>			m_vecCommands;
	public:
		UIMeshBuffer()
			: m_renderable(nullptr)
			, m_vertexBuffer(nullptr)
			, m_indexBuffer(nullptr)
			, m_vertexCount(0)
			, m_indexCount(0)
		{
		}

		void initialize(IContext* _context, IRenderable* _renderable, uint32_t _rectCount);

		void clear() {
			m_vertexCount = 0;
			m_indexCount = 0;
			m_uniformCount = 0;
			m_vecCommands.clear();
		}

		UIUniformElement* getUniformData() {
			return m_uniformBufferMemory.data();
		}

		uint32_t getUniformLength() {
			return m_uniformCount * sizeof(UIUniformElement);
		}

		bool pushVertices(const UIDrawData* _drawData, const UIDrawState& _drawState, const UIDrawState& _lastState);

		void flushMeshBuffer();

		void draw(IRenderPass* _renderPass, IArgument* _argument, uint32_t& _rectCount);
	};

}
