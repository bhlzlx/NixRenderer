#pragma once
#include <NixRenderer.h>
#include "NixUIDrawDataHeap.h"

namespace Nix {

	static const uint32_t FontLayerCount = 4;

	enum UITopologyType {
		UITriangle,
		UIRectangle
	};

	enum UIHoriAlign {
		UIAlignLeft = 0,
		UIAlignRight = 1,
		UIAlignHoriMid = 2		
	};

	enum UIVertAlign {
		UIAlignTop = 0,
		UIAlignBottom = 1,
		UIAlignVertMid = 2
	};

	struct UIVertex {
		float		x; 
		float		y;			// screen space position( x, y )
		// float	z;			// may use a depth param for 3d depth test
		float		u; 
		float		v;			// texture coordinate( u, v )
		float		layer;		// texture array layer
		uint32_t	color;		// color mask
		//
	};

	struct UIDrawState {
		Nix::Scissor			scissor;
		void setScissor(const Nix::Scissor& _scissor) {
			scissor = _scissor;
		}
		bool operator == (const UIDrawState& _state) const {
			if (memcmp(this, &_state, sizeof(*this)) == 0) {
				return true;
			}
			return false;
		}
	};

	struct UIDrawData {
		DrawDataMemoryHeap::Allocation			vertexBufferAllocation;
		UITopologyType							type;
		uint32_t								primitiveCount;
		uint32_t								primitiveCapacity;
		//
		Nix::Rect<int>							area;
	};

	struct UIDrawBatch {
		//uint32_t				vertexBufferIndex;	// vertex buffer index, should be zero currently
		uint32_t				vertexOffset;		// vertex offset
		//uint32_t				indexBufferIndex;	// index buffer index
		uint32_t				indexOffset;		// index buffer offset
		uint16_t				elementCount;		// `element count` param of the `draw element` function
		UIDrawState				state;
	};

}
