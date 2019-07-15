#include <NixRenderer.h>
#include <cassert>
#include <cstdint>
#include <vector>
#include <string>
#include <map>

namespace Nix {

	enum TextureNodeFlags {
		LeftNode = 1 << 0,
		RightNode = 1 << 1,
		Taken = 1 << 2
	};

	enum InsertRetFlags {
		InsertSucceed = 1 << 0,
		InsertNeedClear = 1 << 1
	};

	typedef uint16_t InsertRetFlag;
	typedef uint16_t TextureNodeFlag;

	class TextureNode;

	NIX_API_DECL TextureNode* allocNewNode(uint16_t _x, uint16_t _y, uint16_t _w, uint16_t _h, uint16_t _flags, TextureNode* _P);
	NIX_API_DECL void freeTextureNode(TextureNode* _node);

	class NIX_API_DECL TextureNode {
		friend class TexturePacker;
	private:
		// 16 bit unsigned integer is enough
		uint16_t x;
		uint16_t y;
		//
		uint16_t width;
		uint16_t height;
		//
		uint16_t flags;
		//
		TextureNode* parent;
		TextureNode* left;
		TextureNode* right;
		//
	public:
		TextureNode();
		TextureNode(uint16_t _x, uint16_t _y, uint16_t _w, uint16_t _h, uint16_t _flags, TextureNode* _parent);
		TextureNodeFlag insert( uint16_t _w, uint16_t _h, uint16_t& x_, uint16_t& _y );
	private:
		inline InsertRetFlag spliteVertical(uint16_t _w, uint16_t _h, uint16_t& x_, uint16_t& y_ ) {
			x_ = this->x;
			y_ = this->y;
			//
			uint16_t leftX = this->x;
			uint16_t leftY = this->y + _h;
			uint16_t leftW = _w;
			uint16_t leftH = this->height - _h;

			uint16_t rightX = this->x + _w;
			uint16_t rightY = this->y;
			uint16_t rightW = this->width - _w;
			uint16_t rightH = this->height;
			//
			this->left = allocNewNode(leftX, leftY, leftW, leftH, TextureNodeFlags::LeftNode, this);
			this->right = allocNewNode(rightX, rightY, rightW, rightH, TextureNodeFlags::RightNode, this);

			InsertRetFlag flag = InsertRetFlags::InsertSucceed;

			if (!this->left && !this->right) {
				flag |= InsertNeedClear;
			}

			return flag;
		}

		inline InsertRetFlag spliteHorizontal(uint16_t _w, uint16_t _h, uint16_t& x_, uint16_t& y_) {
			x_ = this->x;
			y_ = this->y;
			//
			uint16_t leftX = this->x + _w;
			uint16_t leftY = this->y;
			uint16_t leftW = this->width - _w;
			uint16_t leftH = _h;

			uint16_t rightX = this->x;
			uint16_t rightY = this->y + _h;
			uint16_t rightW = this->width;
			uint16_t rightH = this->height - _h;
			//
			this->left = allocNewNode(leftX, leftY, leftW, leftH, TextureNodeFlags::LeftNode, this);
			this->right = allocNewNode(rightX, rightY, rightW, rightH, TextureNodeFlags::RightNode, this);

			InsertRetFlag flag = InsertRetFlags::InsertSucceed;

			if (!this->left && !this->right) {
				flag |= InsertNeedClear;
			}

			return flag;
		}
	};

	class NIX_API_DECL TexturePacker {
	private:
		TextureNode		m_headNode;
		ITexture*		m_texture;
		uint32_t		m_layer;
		std::map< std::string, Nix::Rect<uint16_t> > m_record;
	public:
		TexturePacker( ITexture* _texture, uint32_t _layer ):
			m_texture(_texture)
			, m_layer(_layer)
		{
			m_headNode.left = nullptr;
			m_headNode.right = allocNewNode(0, 0, _texture->getDesc().width, _texture->getDesc().height, TextureNodeFlags::RightNode, nullptr );
		}
		bool insert( const char * _key, const uint8_t* _bytes, uint32_t _length, uint16_t _width, uint16_t _height, Nix::Rect<uint16_t>& rect_ );
		InsertRetFlag insert( const char* _key, uint16_t _width, uint16_t _height, uint16_t& x_, uint16_t& y_ );
	};
};

extern "C" {
	NIX_API_DECL Nix::TexturePacker* CreateTexturePacker( Nix::ITexture* _texture, uint32_t _layer );
	typedef Nix::TexturePacker* (*PFN_CREATE_TEXTURE_PACKER)(Nix::ITexture* _texture, uint32_t _layer);
}