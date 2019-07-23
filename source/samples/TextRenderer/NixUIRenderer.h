#include <NixRenderer.h>
#include <nix/io/archieve.h>
#include <nix/memory/BuddySystemAllocator.h>
#include <MemoryPool/C-11/MemoryPool.h>
#include <cassert>

namespace Nix {

	static const uint32_t FontLayerCount = 4;

	struct UIVertex {
		float x; float y; // screen space position( x, y )
		float u; float v; float t; // texture coordinate( u, v )
	};

	/*
	*	The `vertex buffer` & `index buffer` in `PrebuildDrawData` handled by control object should be allocated in a heap,
	*	so that, we can get a better memory management, here we use the `buddy system allocator` management method.
	*/

	/*
	*	considered that one rect need 4 vertex and one vertex should handle at lease 4 float( [x,y], [u,v] )
	*	we assumed that a minimum memory heap an supply at least 8196 characters, each character possesses a rect.
	*   so, we can calculate the minimum heap size 8196(character count) * 4(vertex count) * 4( float count ) * 4( float size )
	*   8196 * 4 * 4 * 4 = 512 KB = 0.5MB
	*   but a minimum control, use only 64 bytes, for a 512KB heap, it will take 13 level binary heap
	*   so we should have a smaller heap to handle the smaller allocation
	*	512 * 64 = 32 KB
	*/

	class PrebuildBufferMemoryHeap {
	public:
		struct Allocation {
			BuddySystemAllocator*	allocator;
			uint32_t				size;
			uint16_t				allocateId;
			uint8_t*				ptr;
		};
	private:
		struct Heap {
			BuddySystemAllocator	allocator;
			uint8_t*				memory;
		};
		std::vector< Heap >			m_heapCollection[2];
	public:
		PrebuildBufferMemoryHeap() {
		}
		Allocation allocate(uint32_t _rectNum) {
			uint32_t loc = _rectNum < 16 ? 0 : 1;
			std::vector< Heap >& heaps = m_heapCollection[loc];

			size_t offset;
			uint16_t allocateId;
			for (auto& heap : heaps) {
				bool allocRst = heap.allocator.allocate(_rectNum * sizeof(UIVertex) * 4, offset, allocateId);
				if (allocRst) {
					Allocation allocation;
					allocation.allocateId = allocateId;
					allocation.allocator = &heap.allocator;
					allocation.size = _rectNum * sizeof(UIVertex) * 4;
					allocation.ptr = heap.memory + offset;
					return allocation;
				}
			}
			heaps.resize(heaps.size() + 1);
			//
			Heap& heap = heaps.back();

			uint32_t heapSizes[] = {
				sizeof(UIVertex) * 512,
				sizeof(UIVertex) * 512 * 16
			};

			uint32_t minSizes[] = {
				sizeof(UIVertex),
				sizeof(UIVertex) * 16
			};
			heap.allocator.initialize(heapSizes[loc], minSizes[loc]);
			heap.memory = new uint8_t[heapSizes[loc]];
			bool allocRst = heap.allocator.allocate(_rectNum * sizeof(UIVertex) * 4, offset, allocateId);
			assert(allocRst);
			Allocation allocation;
			allocation.allocateId = allocateId;
			allocation.allocator = &heap.allocator;
			allocation.size = _rectNum * sizeof(UIVertex) * 4;
			allocation.ptr = heap.memory + offset;
			return allocation;
		}

		void free(const Allocation& _allocation){
			_allocation.allocator->free(_allocation.allocateId);
		}
	};

	/*
	// UI Mesh 无需过度做管理分配，唯一需要做的就是保证 mesh 大小满足一帧绘制所需
	// 所以 UI Mesh 我直接从默认 VertexBuffer 分配器里分配
	// 如果有一帧渲染数量突然大于这个容量，那么，回收这个vertex buffer, 分配新的 vertex buffer, 全部在默认分配器上分配
	*/

	/*
	 * 因为我们需要对 UI mesh 更新，UI mesh 的顶点设置为持续映射的，但是更新我们实际是一个控件一个控件的写入，并不是一次性
	 * 所以为了效率，我们应该在 CPU 可访问的主存端再创建一个缓存，这个缓存是一块连续内存,用来存这一帧所需要更新的所有顶点
	 * 控件上持有的 `PrebuildDrawData` 并不是连续的，所以我们需要在重新构建的时候把这些非连续的变成连续的，就可以方便合并drawcall了
	 */

	class UIMeshManager {
		// for vertex buffer and index buffer, a vector for a vertex buffer maybe used only for one frame
		// because maybe one vertex buffer is not enough for a frame rendering
	private:
		// @ m_vertexBufferPM : vertex buffer objects with `persistent mapping` feature
		std::vector<IBuffer*>						m_vertexBufferPM[MaxFlightCount];
		// @ m_indexBuffer : index buffer objects with static indices data
		std::vector<IBuffer*>						m_indexBuffer[MaxFlightCount];
		//
		std::vector<std::vector<UIVertex>>			m_vertexBufferRebuild;
	public:
	};

	// UI 渲染器使用的 descriptor set 都是同一个！
	// 即 字体纹理，普通UI的图像纹理都统统打包成 texture 2d array
	// 字体使用  R8_UNORM texture 2d array
	// 普通纹理使用 RGBA8_UNORM texture2d array
	// UI 渲染器使用的 vertex buffer object 也应该使用一个！
	// 创建足够大的 vertex/index buffer allocator

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
			if (vertexBufferIndex == _command.vertexBufferIndex && state == _command.state ) {
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
		// vertex buffer cached by control
		PrebuildBufferMemoryHeap::Allocation	vertexBufferAllocation;
		// draw resource reference by control
		DrawCommand								drawCommand;
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

		struct ImageDraw {
			UIVertex		TL; // top left
			UIVertex		BL; // bottom left
			UIVertex		BR; // bottom right
			UIVertex		TR; // top right
			//
			uint32_t		layer; // image layer of the texture array
		};
	private:
		PrebuildBufferMemoryHeap	m_vertexMemoryHeap;
	public:
		UIRenderer() {

		}

		PrebuildDrawData* build( const TextDraw& _draw );
		PrebuildDrawData* build( const ImageDraw* _pImages, uint32_t _count );
		PrebuildDrawData* build( const ImageDraw* _pImages, uint32_t _count, const TextDraw& _draw );
	};


}