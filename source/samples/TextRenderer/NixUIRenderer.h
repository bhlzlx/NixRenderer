#include <NixRenderer.h>

namespace Nix {

	class UIMeshManager {
	private:
	public:
	};

	// UI 渲染器使用的 descriptor set 都是同一个！
	// 即 字体纹理，普通UI的图像纹理都统统打包成 texture 2d array
	// 字体使用  R8_UNORM texture 2d array
	// 普通纹理使用 RGBA8_UNORM texture2d array
	// UI 渲染器使用的 vertex buffer object 也应该使用一个！
	// 创建足够大的 vertex/index buffer allocator

	enum UITexType {
		UITexImage = 0,
		UITexFont = 1
	};

	struct DrawState {
		Nix::Rect<uint16_t>		clip;				//
		float					alpha;				//
		bool operator == (const DrawState& _state) const {
			if (memcmp(this, &_state, sizeof(*this)) == 0) {
				return true;
			}
			return false;
		}
	};

	// 合并策略
	// 如果一个控件需要更新，那么向上查找它的父控件，一直查到一个**合并结点**，并更新这个合并结点的所有**非合并结点**类型的子控件
	// 因此！如果一个控件有一批子控件，这些子控件的合并节点最好是连续的，如果合并节点之前有一个非合并节点，那么就会影响drawcall的合并
	// enum {
	//		DontCombine = 0, // 默认不合并，生成默认的 draw command
	//		Combine = 1, // 遍历子控件，但不包括标记为**合并**的子控件，按顺序尽量分配连续的vbo,ibo, 
	//		ForceCombine = 2, // 遍历所有子控件，按顺序尽量分配连续的vbo,ibo
	//	};
	//
	//  - node( 非合并或者合并或者强制所有子控件合并 )
	//	      - 合并标记
	//	      - 合并标记
	//	      - 合并标记
	//	      - 非合并标记
	//	      - 非合并标记

	struct DrawCommand {
		uint32_t				texType;			// indicate whether it's drawing a set of image or a tile of text
		uint32_t				texLayer;			// indicate the layer of the texture array
		uint32_t				vertexBufferIndex;	// vertex buffer index, should be zero currently
		uint32_t				vertexOffset;		// vertex offset
		uint32_t				indexBufferIndex;	// index buffer index
		uint32_t				indexOffset;		// index buffer offset
		uint16_t				elementCount;		// `element count` param of the `draw element` function
		//
		DrawState				state;
		//
		bool compatible(const DrawCommand& _command) const {
			if (vertexBufferIndex == _command.vertexBufferIndex && indexBufferIndex == _command.indexBufferIndex && state == _command.state ) {
				return true;
			}
			return false;
		}
	};

	class Widget {
	private:
		Widget*					m_parent;
		Nix::Rect<int16_t>		m_rect;
	public:
	};

	struct PrebuildDrawData {
		DrawState		state;
		uint8_t*		vertexBuffer;
		uint32_t		vertexBufferLength;
		uint8_t*		indexBuffer;
		uint32_t		indexBufferLength;
		DrawCommand*	drawCommand;
	};

	class UIRenderer {
	public:
		//
		struct TextDraw {
			char16_t*				text;
			uint32_t				length;
			uint32_t				fontId;
			uint16_t				fontSize; 
			Nix::Point<int16_t>		original;
			Nix::Rect<int16_t>		sissor;
			float					alpha;
		};
		//
		struct ImageVertex {
			float x; float y; // screen space position( x, y )
			float u; float v; // texture coordinate( u, v )
		};

		struct ImageDraw {
			ImageVertex		TL; // top left
			ImageVertex		BL; // bottom left
			ImageVertex		BR; // bottom right
			ImageVertex		TR; // top right
			//
			uint32_t		layer; // image layer of the texture array
		};
	private:
	public:
		UIRenderer() {
		}

		PrebuildDrawData* drawText( const TextDraw& _draw );
		PrebuildDrawData* drawImage( const ImageDraw& _draw );
	};


}