#include <NixRenderer.h>
#include <cassert>

namespace Nix {

 	struct alignas(16) DXT5Block {
// 		struct AlphaBlock {
// 			uint8_t alphaMax : 8;
// 			uint8_t alphaMin : 8;
// 			uint8_t p11 : 3; uint8_t p12 : 3; uint8_t p13 : 3; uint8_t p14 : 3;
// 			uint8_t p21 : 3; uint8_t p22 : 3; uint8_t p23 : 3; uint8_t p24 : 3;
// 			uint8_t p31 : 3; uint8_t p32 : 3; uint8_t p33 : 3; uint8_t p34 : 3;
// 			uint8_t p41 : 3; uint8_t p42 : 3; uint8_t p43 : 3; uint8_t p44 : 3;
// 		} alpha;
// 		struct ColorBlock {
// 			uint16_t colorMax : 16;
// 			uint16_t colorMin : 16;
// 			uint8_t p11 : 2; uint8_t p12 : 2; uint8_t p13 : 2; uint8_t p14 : 2;
// 			uint8_t p21 : 2; uint8_t p22 : 2; uint8_t p23 : 2; uint8_t p24 : 2;
// 			uint8_t p31 : 2; uint8_t p32 : 2; uint8_t p33 : 2; uint8_t p34 : 2;
// 			uint8_t p41 : 2; uint8_t p42 : 2; uint8_t p43 : 2; uint8_t p44 : 2;
// 		} color;

		uint64_t m_alpha;
		uint64_t m_color;

		inline uint8_t calulateAlphaLevel(uint8_t _min, uint8_t _max, uint8_t _alpha) {
			return ((_alpha - _min) * 0xff / (_max - _min)) >> 4;
		}
		inline uint32_t calculateRelativeWeight(uint32_t _color1, uint32_t _color2) {
			uint32_t weight = 
			_color1 ^= _color2;
			for (int i = 0; i < 3; ++i) {
				weight += (_color2 >> i * 8) & 0xff;
			}
			return weight;
		}
		inline uint8_t calulateColorLevel( uint32_t(&_colorTable)[4] , uint32_t _color) {
			uint8_t level = 0;
			uint32_t weight = -1;
			for (uint32_t i = 0; i < 4; ++i) {
				uint32_t currentWeight = calculateRelativeWeight(_colorTable[i], _color);
				if (weight > currentWeight) {
					weight = currentWeight;
					level = i;
				}
			}
			return level;
		}

		void compressBitmap(uint32_t* _bitmapData, uint32_t _width, uint32_t _height) {
			m_alpha = m_color = 0;
			uint32_t pixelCount = _width * _height; assert(pixelCount == 16);

			uint32_t alphaMin = -1;	uint32_t alphaMax = 0;
			uint32_t redMin = -1;	uint32_t redMax = 0;
			uint32_t greenMin = -1;	uint32_t greenMax = 0;
			uint32_t blueMin = -1;	uint32_t blueMax = 0;
			// find the max & min value of r g b a
			for (uint32_t i = 0; i < pixelCount; ++i) {
				uint32_t pixel = _bitmapData[i];
				if (alphaMin > pixel & 0xff000000) alphaMin = pixel & 0xff000000;
				if (alphaMax < pixel & 0xff000000) alphaMax = pixel & 0xff000000;
				if (blueMin > pixel & 0x00ff0000) blueMin = pixel & 0x00ff0000;
				if (blueMax < pixel & 0x00ff0000) blueMax = pixel & 0x00ff0000;
				if (greenMin > pixel & 0x0000ff00) greenMin = pixel & 0x0000ff00;
				if (greenMax < pixel & 0x0000ff00) greenMax = pixel & 0x0000ff00;
				if (redMin > pixel & 0x000000ff) redMin = pixel & 0x000000ff;
				if (redMax < pixel & 0x000000ff) redMax = pixel & 0x000000ff;
			}
			alphaMin >>= 24; alphaMax >>= 24;
			blueMin >>= 16; blueMax >>= 16;
			greenMin >>= 8; greenMax >>= 8;
			//redMin >>= 3; redMax >>= 3;
			uint16_t colorMax = redMax>>3 | greenMax>>2 | blueMax>>3; // RGB565
			uint16_t colorMin = redMin>>3 | greenMin>>2 | blueMin>>3; // RGB565
			// =====
			m_color |= colorMax;
			m_color |= colorMin << 16;

			uint32_t colorTable[4] = {
				redMax | greenMax << 8 | blueMax << 16,
				(redMax - (redMax - redMin) / 3) | (greenMax - (greenMax - greenMin) / 3) << 8 | (blueMax - (blueMax - blueMin) / 3) << 16,
				(redMin + (redMax - redMin) / 3) | (greenMin + (greenMax - greenMin) / 3) << 8 | (blueMin + (blueMax - blueMin) / 3) << 16,
				redMin | greenMin << 8 | blueMin << 16,
			};

			if (alphaMax == alphaMin) {
				alphaMax = 0xffffffff;
				alphaMin = 0x0;
			}
			m_alpha |= alphaMax;
			m_alpha |= alphaMin << 8;
			// == ´¦Àí alpha / color ==
			// 16 ¸öÏñËØ
			for ( uint32_t pixelIndex = 0; pixelIndex < 16; ++pixelIndex) {
				m_alpha |= calulateAlphaLevel(alphaMin, alphaMax, _bitmapData[pixelIndex] >> 24) << (16 + pixelIndex*3);
				m_color |= calulateColorLevel(colorTable, _bitmapData[pixelIndex]) << (32 + pixelIndex * 2);
			}
		}
	};



	class NIX_API_DECL TexturePacker {
	private:
		ITexture*		m_texture;
		uint32_t		m_layer; // if is texture 2d array
	public:
		void initialize();
		void release();
		bool pack( uint32_t _width, uint32_t _height, const char* _rawData );
	};
};

extern "C" {

	NIX_API_DECL Nix::TexturePacker* CreateTexturePacker( Nix::ITexture* _texture, uint32_t _layer );
	NIX_API_DECL void DestroyTexturePacker(Nix::TexturePacker* _packer );

	typedef Nix::TexturePacker* (*PFN_CREATE_TEXTURE_PACKER)(Nix::ITexture* _texture, uint32_t _layer);
	typedef void (*PFN_CREATE_TEXTURE_PACKER)(Nix::TexturePacker* _packer);
}