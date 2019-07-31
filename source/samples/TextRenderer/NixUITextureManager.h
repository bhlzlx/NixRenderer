#pragma once
#include <NixRenderer.h>
#include <stb_truetype.h>
#include <vector>
#include "TexturePacker/TexturePacker.h"

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
	|   | font layers | control texture | network image |
	|
	=============================================================================*/

	class UITextureManager {
	private:
		// the texture2d array object
		ITexture*					m_texture;
		//
		uint32_t					m_numFontLayer;
		uint32_t					m_numControlLayer;
		uint32_t					m_numNetworkLayer;
	public:
		UITextureManager() {
		}
	};

}