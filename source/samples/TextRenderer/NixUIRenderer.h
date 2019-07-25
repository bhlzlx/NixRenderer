#pragma once
#include <NixRenderer.h>
#include <nix/io/archieve.h>
#include <nix/memory/BuddySystemAllocator.h>
#include <MemoryPool/C-11/MemoryPool.h>
#include <assert.h>

#include "NixFontTextureManager.h"
#include "TexturePacker/TexturePacker.h"

#ifdef _WIN32
#include <Windows.h>
#define OpenLibrary( name ) (void*)::LoadLibraryA(name)
#define CloseLibrary( library ) ::FreeLibrary((HMODULE)library)
#define GetExportAddress( libray, function ) ::GetProcAddress( (HMODULE)libray, function )
#else
#include <dlfcn.h>
#define OpenLibrary( name ) dlopen(name , RTLD_NOW | RTLD_LOCAL)
#define CloseLibrary( library ) dlclose((void*)library)
#define GetExportAddress( libray, function ) dlsym( (void*)libray, function )
#endif

#include "NixUIDefine.h"

/****************************
* control -> Prebuild Draw Data
* Prebuild Draw Data array `push` in to mesh manager => generate `Draw command`
*/

namespace Nix {

	/*
	*	The `vertex buffer` & `index buffer` in `UIDrawData` handled by control object should be allocated in a heap,
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

	class DrawDataMemoryHeap {
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
		DrawDataMemoryHeap() {
		}
		Allocation allocateRects(uint32_t _rectNum) {
			uint32_t loc = _rectNum < 16 ? 0 : 1;
			std::vector< Heap >& heaps = m_heapCollection[loc];
			//
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
	 * UI Mesh 无需过度做管理分配，唯一需要做的就是保证 mesh 大小满足一帧绘制所需
	 * 所以 UI Mesh 我直接从默认 VertexBuffer 分配器里分配
	 * 如果有一帧渲染数量突然大于这个容量，那么，回收这个vertex buffer, 分配新的 vertex buffer, 全部在默认分配器上分配
	 */

	/*
	 * 因为我们需要对 UI mesh 更新，UI mesh 的顶点设置为持续映射的，但是更新我们实际是一个控件一个控件的写入，并不是一次性
	 * 所以为了效率，我们应该在 CPU 可访问的主存端再创建一个缓存，这个缓存是一块连续内存,用来存这一帧所需要更新的所有顶点
	 * 控件上持有的 `UIDrawData` 并不是连续的，所以我们需要在重新构建的时候把这些非连续的变成连续的，就可以方便合并drawcall了
	 */

	enum UITopologyType {
		UITriangle,
		UIRectangle
	};
	struct UIDrawData {
		DrawDataMemoryHeap::Allocation			vertexBufferAllocation;
		UITopologyType							type;
		uint32_t								primitiveCount;
		uint32_t								primitiveCapacity;
		//
		UIDrawState								drawState;
	};

	class UIMeshBuffer {
	private:
		IRenderable*				m_renderable;
		IBuffer*					m_vertexBufferPM;
		std::vector<UIVertex>		m_vertexBufferMemory;
		IBuffer*					m_indexBuffer;
		std::vector<uint16_t>		m_indexBufferMemory;
		uint32_t					m_vertexCount;
		uint32_t					m_indexCount;
		//
		std::vector<UIDrawBatch>	m_vecCommands;
	public:
		void initialize( IContext* _context, IRenderable* _renderable, uint32_t _vertexCount ) {
			auto allocator = _context->createVertexBufferAllocatorPM(0, 0);
			m_vertexBufferPM = _context->createVertexBuffer(nullptr, _vertexCount * sizeof(UIVertex), allocator);
			allocator = _context->createIndexBufferAllocator(0, 0);
			m_indexBuffer = _context->createIndexBuffer(nullptr, _vertexCount * 3 / 2, allocator);
			//
			m_vertexBufferMemory.resize( _vertexCount );
			m_indexBufferMemory.resize(_vertexCount * 3 / 2);
			//
			m_vertexCount = 0;
			m_indexCount = 0;
			//
			m_renderable = _renderable;
			m_renderable->setVertexBuffer(m_vertexBufferPM, 0, 0);
			m_renderable->setIndexBuffer(m_indexBuffer, 0);
		}
		
		void clear() {
			m_vertexCount = 0;
			m_indexCount = 0;
			m_vecCommands.clear();
		}

		bool pushVertices(const UIDrawData* _drawData) {

			uint32_t dcVtxCount = 0;
			uint32_t dcIdxCount = 0;

			if (_drawData->type == UITopologyType::UIRectangle) {
				dcVtxCount = _drawData->primitiveCount * 4;
				dcIdxCount = _drawData->primitiveCount * 4 * 3 / 2;
				//
				if ((dcVtxCount + m_vertexCount) > (uint32_t)m_indexBufferMemory.size() ) {
					return false;
				}
				uint16_t baseIndex = m_vertexCount;
				// copy vertices data
				UIVertex* begin = (UIVertex*)_drawData->vertexBufferAllocation.ptr;
				UIVertex* end = begin + dcVtxCount;
				memcpy(&m_vertexBufferMemory[m_vertexCount], begin, dcVtxCount * sizeof(UIVertex));
				m_vertexCount += dcVtxCount;
				// copy indices data
				for (uint32_t i = 0; i < _drawData->primitiveCount; ++i) {
					uint16_t rcIndices[6] = {
						baseIndex, baseIndex + 1, baseIndex + 2,
						baseIndex + 2, baseIndex + 3, baseIndex
					};
					memcpy(&m_indexBufferMemory[m_indexCount], rcIndices, sizeof(rcIndices));
					baseIndex += 4;
					m_indexCount += 6;
				}
			}
			else {
				dcVtxCount = _drawData->primitiveCount * 3;
				dcIdxCount = _drawData->primitiveCount * 3;
				//
				if ( (dcVtxCount + m_vertexCount) > (uint32_t)m_indexBufferMemory.size()) {
					return false;
				}
				uint16_t baseIndex = m_vertexCount;
				// copy vertices data
				UIVertex* begin = (UIVertex*)_drawData->vertexBufferAllocation.ptr;
				UIVertex* end = begin + dcVtxCount;
				memcpy(&m_vertexBufferMemory[m_vertexCount], begin, dcVtxCount * sizeof(UIVertex));
				m_vertexCount += dcVtxCount;
				// copy indices data
				for (uint32_t i = 0; i < _drawData->primitiveCount; ++i) {
					uint16_t rcIndices[3] = {
						baseIndex, baseIndex + 1, baseIndex + 2	
					};
					memcpy(&m_indexBufferMemory[m_indexCount], rcIndices, sizeof(rcIndices));
					baseIndex += 3;
					m_indexCount += 3;
				}
			}
			// can merge the draw call
			if ( m_vecCommands.size() && m_vecCommands.back().state == _drawData->drawState ) {
				UIDrawBatch& cmd = m_vecCommands.back();
				cmd.elementCount += dcIdxCount;
			}
			else
			{
				UIDrawBatch cmd;
				cmd.elementCount = dcIdxCount;
				cmd.indexOffset = (m_indexCount - dcIdxCount);
				cmd.vertexOffset = (m_vertexCount - dcVtxCount) * sizeof(UIVertex);
				cmd.state = _drawData->drawState;
				m_vecCommands.push_back(cmd);
			}
			return true;
		}

		void flushMeshBuffer() {
			m_vertexBufferPM->setData(m_vertexBufferMemory.data(), m_vertexCount * sizeof(UIVertex), 0);
			m_indexBuffer->setData(m_indexBufferMemory.data(), m_indexCount * sizeof(uint16_t), 0);
		}

		void draw(IRenderPass* _renderPass, IArgument* _argument, IPipeline* _pipeline, float _screenWidth, float _screenHeight ) {
			struct Constants {
				float screenWidth;
				float screenHeight;
				float alpha;
			} constants;

			constants.screenWidth = _screenWidth;
			constants.screenHeight = _screenHeight;

			for ( auto& dc : this->m_vecCommands ) {
				_pipeline->setScissor(dc.state.scissor);
				constants.alpha = dc.state.alpha;
				_argument->setShaderCache(0, &constants, sizeof(constants));
				_renderPass->bindArgument(_argument);
				_renderPass->drawElements(m_renderable, dc.indexOffset, dc.elementCount);
			}
		}
	};

	// UI 渲染器使用的 descriptor set 都是同一个！
	// 即 字体纹理，普通UI的图像纹理都统统打包成 texture 2d array
	// 字体使用  R8_UNORM texture 2d array
	// 普通纹理使用 RGBA8_UNORM texture2d array
	// UI 渲染器使用的 vertex buffer object 也应该使用一个！
	// 创建足够大的 vertex/index buffer allocator

	class Widget {
	private:
		Widget*					m_parent;
		Nix::Rect<int16_t>		m_rect;
	public:
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
			Nix::Scissor			scissor;
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
		static const uint32_t MaxVertexCount = 4096 * 4;
		//
		IContext*					m_context;
		IMaterial*					m_material;
		IPipeline*					m_pipeline;
		IArgument*					m_argument;
		ITexture*					m_uiTexArray;
		// ---------------------------------------------------------------------------------------------------
		// resources
		// ---------------------------------------------------------------------------------------------------
		IArchieve*					m_archieve;
		void*						m_packerLibrary;
		PFN_CREATE_TEXTURE_PACKER	m_createPacker;
		DrawDataMemoryHeap	m_vertexMemoryHeap;
		MemoryPool<UIDrawData> 
									m_prebuilDrawDataPool;
		FontTextureManager			m_fontTexManager;

		// ---------------------------------------------------------------------------------------------------
		// runtime drawing
		// ---------------------------------------------------------------------------------------------------
		std::vector<UIMeshBuffer>					m_meshBuffers[MaxFlightCount];
		std::vector<IRenderable*>					m_renderables[MaxFlightCount];
		uint32_t									m_flightIndex;
		uint32_t									m_meshBufferIndex;
	public:
		UIRenderer() {
		}

		// ---------------------------------------------------------------------------------------------------
		//  build draw data
		// ---------------------------------------------------------------------------------------------------

		bool initialize(IContext* _context, IArchieve* _archieve);
		uint32_t addFont( const char * _filepath );

		// ---------------------------------------------------------------------------------------------------
		//  build draw data
		// ---------------------------------------------------------------------------------------------------

		UIDrawData* build( const TextDraw& _draw, UIDrawData* _oldDrawData );
		//UIDrawData* build( const ImageDraw* _pImages, uint32_t _count );
		//UIDrawData* build( const ImageDraw* _pImages, uint32_t _count, const TextDraw& _draw );

		// ---------------------------------------------------------------------------------------------------
		// runtime drawing
		// ---------------------------------------------------------------------------------------------------
		void beginBuild(uint32_t _flightIndex);
		void buildDrawBatch(const UIDrawData * _drawData);
		void endBuild();
		//
		void render(IRenderPass* _renderPass, float _width, float _height);
	};


}