#pragma once

#include <NixRenderer.h>
#include <nix/io/archieve.h>
#include <assert.h>

#include "NixUIDefine.h"
#include "NixUITextureManager.h"
#include "NixUIDrawDataHeap.h"
#include "NixUIMeshBuffer.h"
#include <MemoryPool/C-11/MemoryPool.h>

namespace Nix {

	class ITexturePacker;
	typedef ITexturePacker* (*PFN_CREATE_TEXTURE_PACKER)(Nix::ITexture* _texture, uint32_t _layer);

	struct UIRenderConfig {
		struct UITextureConfig {
			std::string file;
			std::string	table;
			NIX_JSON(file, table)
		};
		std::vector< std::string >		fonts;
		std::vector< UITextureConfig >	textures;
		NIX_JSON(fonts, textures)
	};

	class Widget {
	private:
		Widget*					m_parent;
		Nix::Rect<int16_t>		m_rect;
	public:
	};

	class UIRenderer {
	public:
		struct PlainTextDraw {
			const char16_t*			text;
			uint32_t				length;
			uint32_t				fontId;
			uint16_t				fontSize;
			uint32_t				colorMask;
			//
			Nix::Rect<float>		rect;
			Nix::UIVertAlign		valign;
			Nix::UIHoriAlign		halign;
		};

		struct RichChar {
			uint16_t font;
			uint16_t code;
			uint16_t size;
			uint32_t color;
		};

		struct RichTextDraw {
			std::vector<RichChar>	vecChar;
			Nix::Rect<float>		rect;
			Nix::UIVertAlign		valign;
			Nix::UIHoriAlign		halign;
		};

		struct ImageDraw {
			Nix::Rect<float>				rect;
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
		//void*						m_packerLibrary;
		//PFN_CREATE_TEXTURE_PACKER	m_createPacker;
		DrawDataMemoryHeap			m_vertexMemoryHeap;
		MemoryPool<UIDrawData> 
									m_drawDataPool;
		UITextureManager			m_textureManager;

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

		bool initialize(IContext* _context, IArchieve* _archieve, const UIRenderConfig& _config );
		void setScreenSize( int _width, int _height );

		inline UITextureManager* getUITextureManager() {
			return &m_textureManager;
		}

		uint32_t addFont( const char * _filepath );

		// ---------------------------------------------------------------------------------------------------
		//  build draw data
		// ---------------------------------------------------------------------------------------------------
		
		/**
		* @brief ���� draw data
		*	����ִ�вü�
		* @param[in] _draw  ��Ҫ�������ı�����
		* @param[in] _oldDrawData �ɵ�draw data�ɱ�����
		* @return ���ع������
		* @see
		*/
		UIDrawData* build( const PlainTextDraw& _draw, UIDrawData* _oldDrawData, Nix::Rect<float>& _rc );

		UIDrawData* build(const RichTextDraw& _draw, bool _autoWrap, UIDrawData* _oldDrawData, Nix::Rect<float>& _rc);

		UIDrawData* build( const ImageDraw* _draws, uint32_t _count, UIDrawData* _oldDrawData );

		/**
		* @brief ����һ�� draw data��һ�㸴����������Ⱦ����Ҫ��Ϊ��Ƶ������λ�õĿؼ�ʹ��
		*	ԭ draw data һ����Ϊһ�����ݴ洢��
		* @param[in] _drawData  ���ƶ���
		* @return ���ظ��ƽ��
		* @see
		*/
		UIDrawData* copyDrawData(UIDrawData* _drawData);
		/**
		* @brief ��һ��  draw data ����λ�ò�������� _to ����Ϊ�գ���ԭ��ִ�б任
		*	
		* @param[in] _draw  ��������
		* @param[in] _offsetX  xλ��
		* @param[in] _offsetY  yλ��
		* @see
		*/
		void transformDrawData( UIDrawData* _draw, float _offsetX, float _offsetY, UIDrawData* _to);

		/**
		* @brief ��һ��  draw data ���вü������������ _output
		*
		* @param[in] _draw  ��������
		* @param[in] _scissor  �ü�����
		* @see
		*/
		void scissorDrawData(UIDrawData* _draw, const Nix::Scissor& _scissor, UIDrawData* _output );
		//
		//UIDrawData* build( const ImageDraw* _pImages, uint32_t _count );
		//UIDrawData* build( const ImageDraw* _pImages, uint32_t _count, const PlainTextDraw& _draw );

		/**
		* @brief ����һ�� draw data
		*
		* @param[in] _draw  ���ٶ���
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