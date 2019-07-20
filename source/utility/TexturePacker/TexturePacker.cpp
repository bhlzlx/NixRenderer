#include "TexturePacker.h"

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

	TextureNode* allocNewNode(uint16_t _x, uint16_t _y, uint16_t _w, uint16_t _h, uint16_t _flags, TextureNode* _P);
	void freeTextureNode(TextureNode* _node);

	class TextureNode {
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
		TextureNodeFlag insert(uint16_t _w, uint16_t _h, uint16_t& x_, uint16_t& _y);
	private:
		inline InsertRetFlag splitVertical(uint16_t _w, uint16_t _h, uint16_t& x_, uint16_t& y_) {
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

		inline InsertRetFlag splitHorizontal(uint16_t _w, uint16_t _h, uint16_t& x_, uint16_t& y_) {
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

	class TexturePacker : public ITexturePacker {
	private:
		TextureNode		m_headNode;
		ITexture*		m_texture;
		uint32_t		m_layer;
		uint32_t		m_pixelSize;
		std::map< std::string, Nix::Rect<uint16_t> > m_record;
	public:
		InsertRetFlag  insert( uint16_t _width, uint16_t _height, uint16_t& x_, uint16_t& y_);
		//
		TexturePacker(ITexture* _texture, uint32_t _layer)
			: m_texture(_texture)
			, m_layer(_layer)
		{
			if (_texture->getDesc().format == NixR8_UNORM) {
				m_pixelSize = 1;
			}
			else if (_texture->getDesc().format == NixRGBA8888_UNORM) {
				m_pixelSize = 4;
			}
			m_headNode.left = nullptr;
			m_headNode.right = allocNewNode(0, 0, _texture->getDesc().width, _texture->getDesc().height, TextureNodeFlags::RightNode, nullptr);
		}
		virtual bool insert(const uint8_t* _bytes, uint32_t _length, uint16_t _width, uint16_t _height, Nix::Rect<uint16_t>& rect_);
	};

	//
	inline TextureNode::TextureNode()
		:x(0)
		, y(0)
		, width(0)
		, height(0)
		, flags(0)
		, parent(nullptr)
		, left(nullptr)
		, right(nullptr) {

	}

	inline TextureNode::TextureNode(uint16_t _x, uint16_t _y, uint16_t _w, uint16_t _h, uint16_t _flags, TextureNode* _parent = nullptr )
		:x(_x)
		, y(_y)
		, width(_w)
		, height(_h)
		, flags(_flags)
		, parent(_parent)
		, left(nullptr)
		, right(nullptr)
	{
	}

	TextureNodeFlag TextureNode::insert(uint16_t _w, uint16_t _h, uint16_t& x_, uint16_t& y_ )
	{
		TextureNodeFlag flag = 0;
		if (this->width >= _w && this->height >= _h) {
			this->flags |= Taken;
			if (this->width > this->height) {
				// split vertical
				flag = this->splitVertical(_w, _h, x_, y_);
			} else {
				// split horizontal
				flag = this->splitHorizontal(_w, _h, x_, y_);
			}
		}
		else
		{
			return flag;
		}
		return flag;
	}

	TextureNode* allocNewNode(uint16_t _x, uint16_t _y, uint16_t _w, uint16_t _h, uint16_t _flags, TextureNode* _parent ) {
		if (_w < 4 || _h < 4) {
			return nullptr;
		}
		return new TextureNode(_x, _y, _w, _h, _flags, _parent);
	}
	void freeTextureNode(TextureNode* _node)
	{
		delete _node;
	}

	bool TexturePacker::insert( const uint8_t* _bytes, uint32_t _length, uint16_t _width, uint16_t _height, Nix::Rect<uint16_t>& rect_)
	{
		auto realW = _width + 1;
		auto realH = _height + 1;
		//
		InsertRetFlag flag = insert( realW, realH, rect_.origin.x, rect_.origin.y);
		if (flag & InsertSucceed) {
			rect_.size.width = _width;
			rect_.size.height = _height;
			TextureRegion region;
			region.baseLayer = m_layer;
			region.mipLevel = 0;
			region.offset.x = rect_.origin.x;
			region.offset.y = rect_.origin.y;
			region.offset.z = 0;
			region.size.depth = 1;
			region.size.width = rect_.size.width;
			region.size.height = rect_.size.height;
			m_texture->updateSubData(_bytes, _length, region);
			return true;
		}
		else {
			return false;
		}
	}
	InsertRetFlag TexturePacker::insert( uint16_t _width, uint16_t _height, uint16_t& x_, uint16_t& y_)
	{
		this->m_headNode.right;
		std::vector<TextureNode*> nodes;
		nodes.push_back(this->m_headNode.right);

		InsertRetFlag retFlags = 0;
		TextureNode* lastNode = nullptr;

		while (nodes.size()) {
			auto node = nodes.back();
			nodes.pop_back();
			if (node->flags & TextureNodeFlags::Taken) {
				if (node->right) {
					nodes.push_back(node->right);
				}
				if (node->left) {
					nodes.push_back(node->left);
				}
			}
			else {
				retFlags = node->insert(_width, _height, x_, y_);
				if (retFlags & InsertRetFlags::InsertSucceed) {
					lastNode = node;
					break;
				}
			}
		}

		if (retFlags & InsertRetFlags::InsertNeedClear) {
			assert(lastNode);
			TextureNode* nodeNeedFree = lastNode;
			while (nodeNeedFree) {
				if (nodeNeedFree->flags & TextureNodeFlags::LeftNode) {
					auto parent = nodeNeedFree->parent;
					if (parent) {
						parent->left = nullptr;
						freeTextureNode(nodeNeedFree);
						if (!parent->right) {
							nodeNeedFree = parent;
						}
						else {
							break;
						}
					}else{
						break;
					}
				}
				else if(nodeNeedFree->flags & TextureNodeFlags::RightNode){
					auto parent = nodeNeedFree->parent;
					if (parent) {
						parent->right = nullptr;
						freeTextureNode(nodeNeedFree);
						if (!parent->left) {
							nodeNeedFree = parent;
						}
						else {
							break;
						}
					} else {
						break;
					}
				}
			}
		}
		return retFlags;
	}
}

Nix::ITexturePacker* CreateTexturePacker(Nix::ITexture* _texture, uint32_t _layer)
{
	switch (_texture->getDesc().format) {
	case Nix::NixR8_UNORM:
	case Nix::NixRGBA8888_UNORM: {
		break;
	}
	default:
		return nullptr;
	}
	Nix::TexturePacker* packer = new Nix::TexturePacker(_texture, _layer);
	return packer;
}
