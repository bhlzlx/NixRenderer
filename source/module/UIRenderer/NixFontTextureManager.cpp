#include "NixFontTextureManager.h"
#include <nix/io/archieve.h>
#include <stb_truetype.h>

namespace Nix {

	static const float DPI = 96;

	bool FontTextureManager::initialize(IContext* _context, IArchieve* _archieve, ITexture* _texture, PFN_CREATE_TEXTURE_PACKER _creator, uint32_t _layerBegin, uint32_t _layerEnd)
	{
		const Nix::TextureDescription& desc = _texture->getDesc();
		m_texture = _texture;
		if (!m_texture) {
			return false;
		}
		// initialize the texture packers
		for (uint16_t i = _layerBegin; i <= _layerEnd ; ++i) {
			ITexturePacker * packer = _creator(m_texture, i);
			assert(packer);
			m_vecTexturePacker.push_back(packer);
		}
		// initialize the glyph cache
		m_oneChannelGlyph.resize(64 * 64);
		m_fourChannelGlyph.resize(64 * 64);
		//
		m_archieve = _archieve;
		m_layerRange[0] = _layerBegin;
		m_layerRange[1] = _layerEnd;
		return true;
	}

	uint32_t FontTextureManager::addFont(const char* _fontFile)
	{
		IFile* file = m_archieve->open(_fontFile);
		if (!file) {
			return -1;
		}
		FontInfo font;

		IFile* buffer = CreateMemoryBuffer(file->size());
		file->read(buffer->size(), buffer);
		file->release();
		int error = stbtt_InitFont(&font.handle, (const unsigned char*)buffer->constData(), 0);
		if (!error) {
			return -1;
		}

		int x0, x1, y0,y1 ;
		stbtt_GetFontBoundingBox(&font.handle, &x0, &y0, &x1, &y1);
		font.maxHeight = y1 - y0;
		stbtt_GetFontVMetrics(&font.handle, &font.ascent, &font.descent, &font.lineGap);
		m_vecFont.push_back(font);
		return (uint32_t)m_vecFont.size() - 1;
	}

	void FontTextureManager::getLineHeight(uint8_t _fontId, uint8_t _fontSize, float& height_, float& baseLine_)
	{
		auto& font = m_vecFont[_fontId];
		float scale = getFontScaling(_fontId, _fontSize);
		height_ = roundf(font.maxHeight * scale);
		baseLine_ = roundf((-font.descent * scale) + (font.maxHeight - font.ascent + font.descent)* scale * ((float)-font.descent/(float)(font.ascent - font.descent)));
	}

	float FontTextureManager::getFontScaling(uint8_t _fontId, uint8_t _fontSize)
	{
		float fontSize = _fontSize * DPI / 72.0f;
		auto it = m_vecFont[_fontId].scalingTable.find(_fontSize);
		if (it == m_vecFont[_fontId].scalingTable.end()) {
			float scale = stbtt_ScaleForPixelHeight(&m_vecFont[_fontId].handle, fontSize);
			m_vecFont[_fontId].scalingTable[_fontSize] = scale;
			return scale;
		}
		return it->second;
	}

	const CharactorInfo& FontTextureManager::getCharactor(const CharKey& _c)
	{
		// TODO: 在此处插入 return 语句
		//int i, j, baseline, ch = 0;
		assert(_c.fontId < m_vecFont.size());
		auto& font = m_vecFont[_c.fontId];
		auto& mappingTable = font.charTable;
		auto& fontHandle = font.handle;
		auto it = mappingTable.find(_c.key);
		if (it != mappingTable.end()) {
			return it->second;
		}
		//float xpos = 2; // leave a little padding in case the character extends left
		float scale = 1.0f;
		float fontSize = _c.size * DPI / 72.0f;
		auto scalingIter = font.scalingTable.find(_c.size);
		if (scalingIter == font.scalingTable.end()) {
			//scale = stbtt_ScaleForPixelHeight(&fontHandle, fontSize);
			scale = stbtt_ScaleForMappingEmToPixels(&fontHandle, fontSize);
			font.scalingTable[_c.size] = scale;
		}
		else {
			scale = scalingIter->second;
		}
		//
		//int baseline = (int)(font.ascent * scale);
		int advance, lsb, x0, y0, x1, y1;
		//
		float shiftX = 0.0f;
		float shiftY = 0.0f;
		//
		stbtt_GetCodepointHMetrics(&fontHandle, _c.charCode, &advance, &lsb);
		stbtt_GetCodepointBitmapBoxSubpixel(&fontHandle, _c.charCode, scale, scale, shiftX, shiftY, &x0, &y0, &x1, &y1);
		stbtt_MakeCodepointBitmapSubpixel(&fontHandle,
			&m_oneChannelGlyph[0],
			x1 - x0,
			y1 - y0,
			x1 - x0, // screen width ( stride )
			scale, scale,
			shiftX, shiftY, // shift x, shift y 
			_c.charCode);
		//
		//int advGap = stbtt_GetCodepointKernAdvance(&fontHandle, _c.charCode, '.');
		static CharactorInfo c;
		c.bearingX = x0;
		c.bearingY = -y0;
		c.width = x1 - x0;
		c.height = y1 - y0;
		c.adv = advance * scale;
		//
		size_t pixelNum = (size_t)c.width * c.height;
		//
		for (size_t pixelIndex = 0; pixelIndex < pixelNum; ++pixelIndex) {
			uint32_t color = m_oneChannelGlyph[pixelIndex];
			color = 0xffffff | color<<24 ;
			m_fourChannelGlyph[pixelIndex] = color;
		}
		Nix::Rect<uint16_t> rc;
		if (c.height > 0 && c.width > 0) {
			for (auto texturePacker : m_vecTexturePacker) {
				bool rst = texturePacker->insert((const uint8_t*)&m_fourChannelGlyph[0], c.width * c.height * 4, c.width, c.height, rc);
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