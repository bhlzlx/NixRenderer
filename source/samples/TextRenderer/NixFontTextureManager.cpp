#include "NixFontTextureManager.h"
#include <nix/io/archieve.h>
#include <stb_truetype.h>

namespace Nix {

	bool FontTextureManager::initialize(IContext* _context, IArchieve* _archieve, const Nix::Size3D<uint16_t>& _size, PFN_CREATE_TEXTURE_PACKER _creator)
	{
		Nix::TextureDescription desc;
		desc.depth = _size.depth;
		desc.width = _size.width;
		desc.height = _size.height;
		desc.mipmapLevel = 1;
		desc.type = Nix::Texture2DArray;
		desc.format = NixR8_UNORM;
		// create the texture array with format R8
		m_texture = _context->createTexture( desc );
		if (!m_texture) {
			return false;
		}
		// initialize the texture packers
		for (uint16_t i = 0; i < _size.depth; ++i) {
			ITexturePacker * packer = _creator(m_texture, i);
			assert(packer);
			m_vecTexturePacker.push_back(packer);
		}
		// initialize the glyph cache
		m_glyphCache = new uint8_t[64 * 64];
		//
		m_archieve = _archieve;
		return true;
	}

	uint32_t FontTextureManager::addFont(const char* _fontFile)
	{
		IFile* file = m_archieve->open(_fontFile);
		if (!file) {
			return -1;
		}
		stbtt_fontinfo fhandle;
		IFile* buffer = CreateMemoryBuffer(file->size());
		file->read(buffer->size(), buffer);
		file->release();
		int error = stbtt_InitFont(&fhandle, (const unsigned char*)buffer->constData(), 0);
		if (!error) {
			return -1;
		}
		m_vecFontHandle.push_back(fhandle);
		m_vecMappingTable.resize( m_vecMappingTable.size() + 1);
		return (uint32_t)m_vecFontHandle.size() - 1;
	}

	const CharactorInfo& FontTextureManager::getCharactor(const CharKey& _c)
	{
		// TODO: 在此处插入 return 语句
		//int i, j, baseline, ch = 0;
		assert(_c.fontId < m_vecFontHandle.size());
		auto& mappingTable = m_vecMappingTable[_c.fontId];
		auto& fontHandle = m_vecFontHandle[_c.fontId];
		auto it = mappingTable.find(_c.key);
		if (it != mappingTable.end()) {
			return it->second;
		}
		int baseline;
		int ascent;
		int descent;
		int linegap;
		//float xpos = 2; // leave a little padding in case the character extends left
		float scale = stbtt_ScaleForPixelHeight(&fontHandle, _c.size);
		stbtt_GetFontVMetrics(&fontHandle, &ascent, &descent, &linegap);
		baseline = (int)(ascent * scale);
		int advance, lsb, x0, y0, x1, y1;
		//
		float shiftX = 0.0f;
		float shiftY = 0.0f;
		//
		stbtt_GetCodepointHMetrics(&fontHandle, _c.charCode, &advance, &lsb);
		stbtt_GetCodepointBitmapBoxSubpixel(&fontHandle, _c.charCode, scale, scale, shiftX, shiftY, &x0, &y0, &x1, &y1);
		stbtt_MakeCodepointBitmapSubpixel(&fontHandle,
			&m_glyphCache[0],
			x1 - x0,
			y1 - y0,
			x1 - x0, // screen width ( stride )
			scale, scale,
			shiftX, shiftY, // shift x, shift y 
			_c.charCode);
		CharactorInfo c;
		c.bearingX = x0;
		c.bearingY = -y0;
		c.width = x1 - x0;
		c.height = y1 - y0;
		Nix::Rect<uint16_t> rc;
		if (c.height > 0 && c.width > 0) {
			for (auto texturePacker : m_vecTexturePacker) {
				bool rst = texturePacker->insert(m_glyphCache, c.width * c.height, c.width, c.height, rc);
				c.x = rc.origin.x;
				c.y = rc.origin.y;
				mappingTable[_c.key] = c;
				return mappingTable[_c.key];
			}
		}
		// 其实这里不应该这么写，应该返回一个特殊字符！
		return c;
	}

	void FontTextureManager::reset()
	{
		// 待完成，先测试渲染效果
	}

}