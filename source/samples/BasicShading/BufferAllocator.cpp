#include "BufferAllocator.h"
#include <cassert>

#undef TRUE
#undef FALSE

#define TRUE 1
#define FALSE 0

namespace Nix {

	bool BufferAllocator::initialize(size_t _wholeSize, size_t _minSize) {
		size_t layer = 0;
		size_t miniCount = 1;
		while (miniCount<_minSize) {
			++layer;
			miniCount = (size_t)1 << layer;
		}
		size_t fullCount = miniCount * 2;
		m_minSize = _minSize;
		m_maxIndex = fullCount - 1;
		m_capacity = _wholeSize;
		m_nodeTable.resize(fullCount);
		m_nodeTable[0].hasChild = FALSE;
		m_nodeTable[0].free = TRUE;
		m_nodeTable[1] = m_nodeTable[0];
		for (size_t i = 0; i < m_nodeTable.size(); ++i) {
			m_nodeTable[i].nodeType = (i & 0x1);
		}
		return true;
	}

	size_t BufferAllocator::allocate(size_t _size, size_t& _offset) {
		struct alloc_t {
			size_t layer;
			size_t index;
		};
		std::vector<alloc_t> allocs;
		allocs.push_back({ 0, 1 });
		while (!allocs.empty()) {
			
			alloc_t alloc = allocs.back();
			allocs.pop_back();

			node_t& node = m_nodeTable[alloc.index];
			if (node.free) {
				size_t nodeSize = m_capacity >> alloc.layer;
				if (_size < nodeSize >> 1 && (nodeSize >> 1) >= m_minSize ) {
					// can alloc from it's child node!
					if (!node.hasChild) {
						node.hasChild = true;
						node.leftFree = node.rightFree = true;
						m_nodeTable[alloc.index * 2].free = true;
						m_nodeTable[alloc.index * 2].hasChild = false;
						m_nodeTable[alloc.index * 2 + 1].free = true;
						m_nodeTable[alloc.index * 2 + 1].hasChild = false;
					}
					if (node.rightFree) {
						allocs.push_back({ alloc.layer + 1, alloc.index * 2 + 1 });
					}
					if (node.leftFree) {
						allocs.push_back({ alloc.layer + 1, alloc.index * 2 });
					}
				} else {
					// this node is just fit for the allocation size
					if (!node.hasChild) {
						if (alloc.index > m_maxIndex) {
							return 0;
						}
						// allocate successfully!
						_offset = (alloc.index - ((size_t)1 << alloc.layer) ) * m_capacity / ((size_t)1 << alloc.layer);
						updateTableForAllocate(node, alloc.layer, alloc.index);
						return m_capacity >> alloc.layer;
					}
					else if (node.hasChild) {
						continue;
					}
				}
			} else {
				continue;
			}
		}
		return 0;
	}

	bool BufferAllocator::free(size_t _offset, size_t _capacity) {
		// _offset = (alloc.index - (1 >> alloc.layer) ) * m_capacity / (1 >> alloc.layer);
		assert(_offset % m_minSize == 0);
		assert(_capacity >= m_minSize);

		size_t layer = 0;
		while (_capacity < m_capacity) {
			_capacity = _capacity << 1;
			++layer;
		}
		size_t index = (_offset / (m_capacity / ((size_t)1 << layer))) + ((size_t)1 << layer);
		node_t& node = m_nodeTable[index];
		updateTableForFree(node, layer, index);
		return true;
	}

	bool BufferAllocator::updateTableForAllocate(node_t& _node, size_t _layer, size_t _index )
	{
		_node.free = false;
		_node.hasChild = false;
		_node.leftFree = _node.rightFree = false;

		if (_layer) {
			node_t& parentNode = m_nodeTable[_index / 2];
			if (_node.nodeType == LeftNode) {
				parentNode.leftFree = false;
			}
			else
			{
				parentNode.rightFree = false;
			}
		}

		size_t layer = _layer;
		size_t index = _index;
		while (layer) {
			auto& leftNode = m_nodeTable[(index >> 1) << 1];
			auto& rightNode = m_nodeTable[((index >> 1) << 1) + 1];
			auto& parentNode = m_nodeTable[index / 2];

			parentNode.leftFree = leftNode.free;
			parentNode.rightFree = rightNode.free;

			if (!leftNode.free && !rightNode.free) {
				parentNode.free = false;
				--layer;
				index = index>>1;
			}
			else
			{
				break;
			}
		}
		return true;
	}

	bool BufferAllocator::updateTableForFree(node_t& _node, size_t _layer, size_t _index)
	{
		_node.free = true;
		_node.hasChild = false;
		//
		if (_layer) {
			node_t& parentNode = m_nodeTable[_index / 2];
			if (_node.nodeType == LeftNode) {
				parentNode.leftFree = true;
			} else {
				parentNode.rightFree = true;
			}
		}
		//
		size_t layer = _layer;
		size_t index = _index;
		while (layer) {
			auto& leftNode = m_nodeTable[(index >> 1) << 1];
			auto& rightNode = m_nodeTable[((index >> 1) << 1) + 1];
			auto& parentNode = m_nodeTable[index / 2];
			if (leftNode.free && rightNode.free) {
				parentNode.free = true;
				parentNode.hasChild = false;
				--layer;
				index = index >> 1;
			}
			else
			{
				break;
			}
		}
		return true;
	}

}



