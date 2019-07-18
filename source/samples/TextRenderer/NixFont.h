#pragma once
#include <stb_truetype.h>
#include <map>

namespace Nix {

	class IArchieve;
	class ITexturePacker;
#pragma pack( push, 1 )
	struct FontCharactor {
		union {
			struct {
				uint8_t		fontId;
				uint8_t		size;
				uint16_t	charCode;
			};
			uint32_t key;
		};
	};
	struct CharactorInfo {
		int8_t		width;
		int8_t		height;
		int8_t		bearingX;
		int8_t		bearingY;
		// offset x = bearingX
		// offset y = height - bearingY;
		uint8_t		layer; // layer number of the texture 2d array
		uint16_t	x; // x position in texture layer
		uint16_t	y; // y position in texture layer
	};
	static_assert(sizeof(FontCharactor) == 4, "key size must be 32bit");
#pragma pack(pop)

	class Font {
	private:
		stbtt_fontinfo						m_fontHandle;
		std::map< uint32_t, CharactorInfo > m_mappingTable;
		uint8_t								m_outputData[64 * 64];
		uint8_t								m_id;
		ITexturePacker*						m_texturePacker;
	public:
		Font() {
		}
		bool initialize( IArchieve* _archieve, const char* _filepath, uint8_t _fontID, ITexturePacker* _packer );
		const CharactorInfo& getCharacter( const FontCharactor& _char );
	};

}