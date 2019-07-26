#pragma once
#include <NixRenderer.h>
#include "NixUIDrawDataHeap.h"

namespace Nix {

	static const uint32_t FontLayerCount = 4;

	enum UITopologyType {
		UITriangle,
		UIRectangle
	};

	struct UIVertex {
		float		x; 
		float		y;			// screen space position( x, y )
		// float	z;			// may use a depth param for 3d depth test
		float		u; 
		float		v;			// texture coordinate( u, v )
		float		layer;		// texture array layer
		uint32_t	color;		// color mask
	};

	struct UIDrawState {
		// if inherit scissor is active don't need to
		uint8_t					inheritScissor;
		Nix::Scissor			scissor;
		bool operator == (const UIDrawState& _state) const {
			if (memcmp(this, &_state, sizeof(*this)) == 0) {
				return true;
			}
			return false;
		}
	};

	struct UIDrawData {
		PrebuildBufferMemoryHeap::Allocation	vertexBufferAllocation;
		UITopologyType							type;
		uint32_t								primitiveCount;
		uint32_t								primitiveCapacity;
		//
		UIDrawState								drawState;
	};

	struct UIDrawBatch {
		uint32_t				vertexBufferIndex;	// vertex buffer index, should be zero currently
		uint32_t				vertexOffset;		// vertex offset
		uint32_t				indexBufferIndex;	// index buffer index
		uint32_t				indexOffset;		// index buffer offset
		uint16_t				elementCount;		// `element count` param of the `draw element` function
		UIDrawState				state;
	};

}
