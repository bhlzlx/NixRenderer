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
			uint32_t				colorMask;
			//
			Nix::Rect<int16_t>		rect;
			Nix::UIVertAlign		valign;
			Nix::UIHoriAlign		halign;
		};

		struct ImageDraw {
			Nix::Rect<int16_t>				rect;
			// 不要用 rect<float> 来设置uv, 因为图可能是旋转90度的，所以要传4组uv
			Nix::Point<float>				uv[4];
			float							layer;
			uint32_t						color;
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
									m_drawDataPool;
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
		
		/**
		* @brief 构建 draw data
		*	但不执行裁剪
		* @param[in] _draw  需要构建的文本描述
		* @param[in] _oldDrawData 旧的draw data可被重用
		* @return 返回构建结果
		* @see
		*/
		UIDrawData* build( const TextDraw& _draw, UIDrawData* _oldDrawData );

		UIDrawData* build( const ImageDraw* _draws, uint32_t _count, UIDrawData* _oldDrawData );

		/**
		* @brief 复制一份 draw data，一般复制用来做渲染，主要是为了频繁更新位置的控件使用
		*	原 draw data 一般作为一个备份存储用
		* @param[in] _drawData  复制对象
		* @return 返回复制结果
		* @see
		*/
		UIDrawData* copyDrawData(UIDrawData* _drawData);
		/**
		* @brief 对一个  draw data 进行位置操作，如果 _to 参数为空，则原地执行变换
		*	
		* @param[in] _draw  操作对象
		* @param[in] _offsetX  x位移
		* @param[in] _offsetY  y位移
		* @see
		*/
		void transformDrawData( UIDrawData* _draw, float _offsetX, float _offsetY, UIDrawData* _to);

		/**
		* @brief 对一个  draw data 进行裁剪操作，输出到 _output
		*
		* @param[in] _draw  操作对象
		* @param[in] _scissor  裁剪矩阵
		* @see
		*/
		void scissorDrawData(UIDrawData* _draw, const Nix::Scissor& _scissor, UIDrawData* _output );
		//
		//UIDrawData* build( const ImageDraw* _pImages, uint32_t _count );
		//UIDrawData* build( const ImageDraw* _pImages, uint32_t _count, const TextDraw& _draw );

		/**
		* @brief 销毁一个 draw data
		*
		* @param[in] _draw  销毁对象
		* @see
		*/

		void destroyDrawData( UIDrawData* _draw );

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