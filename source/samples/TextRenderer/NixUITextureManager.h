#pragma once
#include <NixRenderer.h>
#include <stb_truetype.h>
#include <vector>
#include "NixFontTextureManager.h"

namespace Nix {
	
	/* =============================================================================
	|  a class manager all textures that will be used by 'UI' module
	|  there will be several type of textures
	|      1. fonts
	|      2. control skin resource
	|	   3. network downloaded image
	|  all textures are grouped in to a texture2d array, but texture layers may reserved
	|  several layers for `fonts`
	|  `control skin resource` were packed before the resource was published, so it's possessed
	|  fixed number of layers
	|  for `network downloaded image`, one or two 1024x1024 is enough
	|  
	|	physical alignment of the layers
	|
	|				      ======>
	|   | font layers | control layers | network layers |
	|
	=============================================================================*/

	struct UITexture {
		uint16_t	width;
		uint16_t	height;
		float		uv[4][2]; // 4 [u,v] pair
		float		layer;
	};

	class UITextureManager {
	public:
		const static uint32_t UITextureSize = 1024;
	private:
		//
		IContext*							m_context;
		IArchieve*							m_archieve;
		void*								m_texturePackerLibrary;
		// the texture2d array object
		ITexture*							m_texture;
		//
		uint32_t							m_numFontLayer;
		uint32_t							m_numControlLayer;
		uint32_t							m_numNetworkLayer;
		//
		uint32_t							m_freeControlLayer;
		//
		FontTextureManager					m_fontTexManager;
		//
		std::vector<
			std::map<std::string, UITexture>
										>	m_imageTables;
	public:
		UITextureManager() 
		: m_context( nullptr )
		, m_archieve( nullptr )
		, m_texture( nullptr )
		, m_numFontLayer( 0 )
		, m_numControlLayer( 0 )
		, m_numNetworkLayer( 0 )
		{
		}

		inline FontTextureManager* getFontTextureManger() {
			return &m_fontTexManager;
		}
		
		bool initialize( IContext* _context, IArchieve* _archieve, uint32_t _numFontLayer, uint32_t _numControlLayer, uint32_t _numNetworkLayer);

		bool addPackedImage( const char * _jsonTable, const char * _imageFile );
		
		bool getTexture( const std::string& _filename, const UITexture** _handle );
	};

}