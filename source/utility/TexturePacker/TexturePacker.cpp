#include "TexturePacker.h"

namespace Nix {
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
		if (this->width >= _w && this->height >= _h) {
			if (this->width > this->height) {
				// splite vertical
				return this->spliteVertical(_w, _h, x_, y_);
			} else {
				// splite horizontal
				return this->spliteHorizontal(_w, _h, x_, y_);
			}
		}
		else {
			return 0;
		}
	}

	TextureNode* allocNewNode(uint16_t _x, uint16_t _y, uint16_t _w, uint16_t _h, uint16_t _flags, TextureNode* _parent ) {
		if (_x & 1) {
			_x = _x + 1;
			_w = _w - 1;
			if ((_w & ~(1)) < 4) {
				return nullptr;
			}
		}
		if (_y & 1) {
			_y = _y + 1;
			_h = _h - 1;
			if ((_h & ~(1)) < 4) {
				return nullptr;
			}
		}
		return new TextureNode(_x, _y, _w, _h, _flags, _parent);
	}
	void freeTextureNode(TextureNode* _node)
	{
		delete _node;
	}
	bool TexturePacker::insert(const char* _key, const uint8_t* _bytes, uint32_t _length, uint16_t _width, uint16_t _height, Nix::Rect<uint16_t>& rect_)
	{
		InsertRetFlag flag = insert(_key, _width, _height, rect_.origin.x, rect_.origin.y);
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
			m_texture->updateSubData(_key, _length, region);
			return true;
		}
		else {
			return false;
		}
	}
	InsertRetFlag TexturePacker::insert(const char* _key, uint16_t _width, uint16_t _height, uint16_t& x_, uint16_t& y_)
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
		if (retFlags & InsertRetFlags::InsertSucceed) {
			m_record[_key] = {
				{x_, y_}, 
				{_width, _height}
			};
		}
		return retFlags;
	}
}

Nix::TexturePacker* CreateTexturePacker(Nix::ITexture* _texture, uint32_t _layer)
{
	Nix::TexturePacker* packer = new Nix::TexturePacker(_texture, _layer);
	return packer;
}
