#pragma once
#include <NixRenderer.h>
#include <nix/io/archieve.h>
#include <assert.h>

#include "NixUIDefine.h"
#include "NixFontTextureManager.h"
#include "NixUIDrawDataHeap.h"
#include "NixUIMeshBuffer.h"
#include <MemoryPool/C-11/MemoryPool.h>

namespace Nix {

	class ITexturePacker;
	typedef ITexturePacker* (*PFN_CREATE_TEXTURE_PACKER)(Nix::ITexture* _texture, uint32_t _layer);

	class Widget {
	private:
		Widget*					m_parent;
		Nix::Rect<int16_t>		m_rect;
	public:
	};

	enum VertexClipFlagBits {
		AllClipped = 0,
		PartClipped = 1,
		NoneClipped = 2,
	};
	typedef uint8_t VertexClipFlags;

	// UI 渲染器使用的 descriptor set 都是同一个！
	// 即 字体纹理，普通UI的图像纹理都统统打包成 texture 2d array
	// 字体使用  R8_UNORM texture 2d array
	// 普通纹理使用 RGBA8_UNORM texture2d array
	// UI 渲染器使用的 vertex buffer object 也应该使用一个！
	// 创建足够大的 vertex/index buffer allocator

	class UIRenderer {
	public:
		struct TextDraw {
			const char16_t*			text;
			uint32_t				length;
			uint32_t				fontId;
			uint16_t				fontSize;
			//
			uint32_t				colorMask;
		};

		struct ImageDraw {
			UIVertex				vertices[4];
			//
			Nix::Scissor			scissor;
			float					alpha;
		};
	private:
		static const uint32_t MaxVertexCount = 2048 * 4;
		//
		IContext*					m_context;
		IMaterial*					m_material;
		IPipeline*					m_pipeline;
		IArgument*					m_argument;
		ITexture*					m_textureArray;
		//
		int							m_width;
		int							m_height;
		UIDrawState					m_drawState;
		// ---------------------------------------------------------------------------------------------------
		// resources
		// ---------------------------------------------------------------------------------------------------
		IArchieve*					m_archieve;
		void*						m_packerLibrary;
		PFN_CREATE_TEXTURE_PACKER	m_createPacker;
		DrawDataMemoryHeap			m_vertexMemoryHeap;
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
		void setScreenSize( int _width, int _height );
		uint32_t addFont( const char * _filepath );

		// ---------------------------------------------------------------------------------------------------
		//  build draw data
		// ---------------------------------------------------------------------------------------------------

		UIDrawData* build( const TextDraw& _draw, UIDrawData* _oldDrawData );
		//
		UIDrawData* buildAdvanced( UIDrawData* _drawData, const Nix::Scissor& _scissor );
		//UIDrawData* build( const ImageDraw* _pImages, uint32_t _count );
		//UIDrawData* build( const ImageDraw* _pImages, uint32_t _count, const TextDraw& _draw );

		// ---------------------------------------------------------------------------------------------------
		// runtime drawing
		// ---------------------------------------------------------------------------------------------------
		void beginBuild(uint32_t _flightIndex);
		void buildDrawCall(const UIDrawData * _drawData, const UIDrawState& _state );
		void endBuild();
		//
		void render(IRenderPass* _renderPass, float _width, float _height);
	};


}