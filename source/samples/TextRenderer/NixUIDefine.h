#include <stdint.h>
#include <NixRenderer.h>

namespace Nix {
	
	static const uint32_t UITextureArrayCountMax = 4;

	struct UIVertex {
		float x; float y; // screen space position( x, y )
		float u; float v; // texture coordinate( u, v )
		float layer;	  // texture array layer
	};

	struct UIDrawState {
		Nix::Scissor			scissor;
		float					alpha;
		bool operator == (const UIDrawState& _state) const {
			if (memcmp(this, &_state, sizeof(*this)) == 0) {
				return true;
			}
			return false;
		}
	};

	struct UIDrawBatch {
		//uint32_t				vertexBufferIndex;	// vertex buffer index, should be zero currently
		uint32_t				vertexOffset;		// vertex offset
		///uint32_t				indexBufferIndex;	// index buffer index
		uint32_t				indexOffset;		// index buffer offset
		uint16_t				elementCount;		// `element count` param of the `draw element` function
		//
		UIDrawState				state;
	};

}