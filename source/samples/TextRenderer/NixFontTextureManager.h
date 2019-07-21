#include <NixRenderer.h>
#include <stb_truetype.h>
#include "TexturePacker/TexturePacker.h"

namespace Nix {

#pragma pack( push, 1 )
	struct CharKey {
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
	static_assert(sizeof(CharKey) == 4, "key size must be 32bit");
#pragma pack(pop)

	class FontTextureManager {
	private:
		IContext*						m_context;
		IArchieve*						m_archieve;
		ITexture*						m_texture;
		uint32_t 						m_numImageLayer;
		uint32_t						m_numFontLayer;
		//
		std::vector<ITexturePacker*>	m_vecTexturePacker;
		std::vector<stbtt_fontinfo>		m_vecFontHandle;
		std::vector< std::map< uint32_t, CharactorInfo > >
										m_vecMappingTable;
		//
		uint8_t*						m_glyphCache;
	public:
		FontTextureManager() {
		}
		ITexture* getTexture() {
			return m_texture;
		}
		// _size indicate the texture2DArray's size : width/ height/ depth
		bool initialize( IContext* _context, IArchieve* _archieve, const Nix::Size3D<uint16_t>& _size, PFN_CREATE_TEXTURE_PACKER _creator );
		//
		uint32_t addFont( const char * _fontFile );
		//
		const CharactorInfo& getCharactor( const CharKey& _c );
		//
		void reset();
	};
}