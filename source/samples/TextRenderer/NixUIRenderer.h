#include <NixRenderer.h>

namespace Nix {

	class UIMeshManager {
	private:
	public:
	};

	// UI ��Ⱦ��ʹ�õ� descriptor set ����ͬһ����
	// �� ����������ͨUI��ͼ������ͳͳ����� texture 2d array
	// ����ʹ��  R8_UNORM texture 2d array
	// ��ͨ����ʹ�� RGBA8_UNORM texture2d array
	// UI ��Ⱦ��ʹ�õ� vertex buffer object ҲӦ��ʹ��һ����
	// �����㹻��� vertex/index buffer allocator

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

	// �ϲ�����
	// ���һ���ؼ���Ҫ���£���ô���ϲ������ĸ��ؼ���һֱ�鵽һ��**�ϲ����**������������ϲ���������**�Ǻϲ����**���͵��ӿؼ�
	// ��ˣ����һ���ؼ���һ���ӿؼ�����Щ�ӿؼ��ĺϲ��ڵ�����������ģ�����ϲ��ڵ�֮ǰ��һ���Ǻϲ��ڵ㣬��ô�ͻ�Ӱ��drawcall�ĺϲ�
	// enum {
	//		DontCombine = 0, // Ĭ�ϲ��ϲ�������Ĭ�ϵ� draw command
	//		Combine = 1, // �����ӿؼ��������������Ϊ**�ϲ�**���ӿؼ�����˳��������������vbo,ibo, 
	//		ForceCombine = 2, // ���������ӿؼ�����˳��������������vbo,ibo
	//	};
	//
	//  - node( �Ǻϲ����ߺϲ�����ǿ�������ӿؼ��ϲ� )
	//	      - �ϲ����
	//	      - �ϲ����
	//	      - �ϲ����
	//	      - �Ǻϲ����
	//	      - �Ǻϲ����

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