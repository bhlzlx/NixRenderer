#pragma once
#include <NixRenderer.h>
#include "NixUIDrawDataHeap.h"

namespace Nix {

	static const uint16_t UITextureSize = 1024;

	enum VertexClipFlagBits {
		AllClipped = 0,
		PartClipped = 1,
		NoneClipped = 2,
	};

	typedef uint8_t VertexClipFlags;
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
		//float	    z;			// may use a depth param for 3d depth test
		float		u;
		float		v;			// texture coordinate( u, v )
		uint32_t	uniformIndex;
	};

	struct alignas(16) UIUniformElement {
		struct {
			uint32_t	layer; // texture array layer
			uint32_t	color; // color mask
		};
	};
	static_assert(sizeof(UIUniformElement) == 16, "");

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
		std::vector<UIUniformElement>			uniformData;
		UITopologyType							type;
		uint32_t								primitiveCount;
		uint32_t								primitiveCapacity;
	};

	struct UIDrawBatch {
		uint32_t				vertexOffset;		// vertex offset
		uint32_t				indexOffset;		// index buffer offset
		uint16_t				elementCount;		// `element count` param of the `draw element` function
		UIDrawState				state;
	};

}
