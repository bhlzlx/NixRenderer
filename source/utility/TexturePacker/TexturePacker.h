#include <NixRenderer.h>
#include <cassert>
#include <cstdint>
#include <vector>
#include <string>
#include <map>

namespace Nix {

	class NIX_API_DECL ITexturePacker {
	public:
		virtual bool insert( const uint8_t* _bytes, uint32_t _length, uint16_t _width, uint16_t _height, Nix::Rect<uint16_t>& rect_) = 0;
	};
};

extern "C" {
	NIX_API_DECL Nix::ITexturePacker* CreateTexturePacker( Nix::ITexture* _texture, uint32_t _layer );
	typedef Nix::ITexturePacker* (*PFN_CREATE_TEXTURE_PACKER)(Nix::ITexture* _texture, uint32_t _layer);
}