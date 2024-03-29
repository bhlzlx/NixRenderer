﻿#pragma once
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

		struct RichItem {
			enum Type {
				Charactor,
				Image,
			};
			uint8_t type;
			union {
				struct {
					uint16_t font;
					uint16_t code;
					uint16_t size;
					uint32_t color;
				};
				struct {
					const char *	image;
					uint8_t			width;
					uint8_t			height;
				};
			};
		};

		struct RichTextDraw {
			std::vector<RichChar>	vecChar;
			Nix::Rect<float>		rect;
			Nix::UIVertAlign		valign; // line alignment
			Nix::UIHoriAlign		halign; // line alignment
			Nix::UIVertAlign		calign; // text content alignment
		};

		struct RichDraw {

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
		IBuffer*					m_uniformBuffer;
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

		bool initialize(IContext* _context, IArchieve* _archieve, const UIRenderConfig& _config);
		void setScreenSize(int _width, int _height);

		inline UITextureManager* getUITextureManager() {
			return &m_textureManager;
		}

		uint32_t addFont(const char * _filepath);

		// ---------------------------------------------------------------------------------------------------
		//  build draw data
		// ---------------------------------------------------------------------------------------------------

		UIDrawData* build(const PlainTextDraw& _draw, UIDrawData* _oldDrawData, Nix::Rect<float>& _rc);
		UIDrawData* build(const RichTextDraw& _draw, bool _autoWrap, UIDrawData* _oldDrawData, Nix::Rect<float>& _rc);
		UIDrawData* build(const ImageDraw* _draws, uint32_t _count, UIDrawData* _oldDrawData);
		//UIDrawData* build(uint32_t _numTri, UIDrawData* _oldDrawData);
		//UIDrawData* buildLines(Nix::Point<float>* _points, uint32_t _numLine);


		UIDrawData* copyDrawData(UIDrawData* _drawData);
		void translateDrawData(UIDrawData* _draw, float _offsetX, float _offsetY, UIDrawData* _to);
		void rotateDrawData(UIDrawData* _draw, const Nix::Point<float>& _anchor, float _angle, UIDrawData* _to);
		void transformDrawData(UIDrawData* _draw, const Nix::Point<float>& _anchor, float _angle, float _scale, UIDrawData* _to);
		void scissorDrawData(UIDrawData* _draw, const Nix::Scissor& _scissor, UIDrawData* _output);
		void destroyDrawData(UIDrawData* _draw);

		// ---------------------------------------------------------------------------------------------------
		// runtime drawing
		// ---------------------------------------------------------------------------------------------------
		void beginBuild(uint32_t _flightIndex);
		void buildDrawCall(const UIDrawData * _drawData, const UIDrawState& _state);
		void endBuild();
		//
		void render(IRenderPass* _renderPass, float _width, float _height);
	};


}
