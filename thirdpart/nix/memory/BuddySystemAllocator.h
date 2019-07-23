#include <vector>
#include <list>

namespace Nix {

	class BuddySystemAllocator {
		enum node_type_t {
			LeftNode = 0,
			RightNode
		};
		struct node_t {
			uint8_t free : 1;
			uint8_t hasChild : 1;
			uint8_t leftFree : 1;
			uint8_t rightFree : 1;
			uint8_t nodeType : 1;
		};
	private:
		std::vector<node_t> m_nodeTable; // m_nodeTable[0] is not used
		//
		size_t m_capacity;
		size_t m_minSize;
		size_t m_maxIndex;
	public:
		BuddySystemAllocator() 
		: m_capacity(0)
		, m_minSize(0)
		, m_maxIndex(0) {
		}
		bool initialize(size_t _wholeSize, size_t _minSize );
		bool allocate(size_t _size, size_t& offset_, uint16_t& _id );
		bool free( uint16_t _id );
	private:
		bool updateTableForAllocate( node_t& _node, size_t _layer, size_t _index);
		bool updateTableForFree(node_t& _node, size_t _layer, size_t _index);
	};

}