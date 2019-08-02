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
		IRenderable* m_renderable;
		IBuffer* m_vertexBufferPM;
		std::vector<UIVertex>		m_vertexBufferMemory;
		IBuffer* m_indexBuffer;
		std::vector<uint16_t>		m_indexBufferMemory;
		uint32_t					m_vertexCount;
		uint32_t					m_indexCount;
		//
		std::vector<UIDrawBatch>	m_vecCommands;
	public:
		void initialize(IContext* _context, IRenderable* _renderable, uint32_t _vertexCount);

		void clear() {
			m_vertexCount = 0;
			m_indexCount = 0;
			m_vecCommands.clear();
		}

		bool pushVertices(const UIDrawData* _drawData, const UIDrawState& _drawState, const UIDrawState& _lastState );

		void flushMeshBuffer();

		void draw(IRenderPass* _renderPass, IArgument* _argument, IPipeline* _pipeline, float _screenWidth, float _screenHeight);
	};

}