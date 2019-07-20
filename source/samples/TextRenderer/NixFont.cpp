#include "NixFont.h"
#include <TexturePacker/TexturePacker.h>
#include <nix/io/archieve.h>

namespace Nix {

	bool Font::initialize( IArchieve* _archieve, const char* _filepath, uint8_t _fontID, ITexturePacker* _packer)
	{
		IFile* file = _archieve->open(_filepath);
		if (!file) {
			return false;
		}
		IFile* buffer = CreateMemoryBuffer(file->size());
		file->read(buffer->size(), buffer);
		file->release();
		int error = stbtt_InitFont(&m_fontHandle, (const unsigned char*)buffer->constData(), 0);
		m_id = _fontID;
		m_texturePacker = _packer;
		m_outputData = new uint8_t[64*64];
		return !!error;
	}

	const CharactorInfo& Font::getCharacter(const FontCharactor& _char)
	{
		//int i, j, baseline, ch = 0;
		auto it = m_mappingTable.find(_char.key);
		if (it != m_mappingTable.end()) {
			return it->second;
		}
		int baseline;
		int ascent;
		int descent;
		int linegap;
		//float xpos = 2; // leave a little padding in case the character extends left
		float scale = stbtt_ScaleForPixelHeight(&m_fontHandle, _char.size);
		stbtt_GetFontVMetrics(&m_fontHandle, &ascent, &descent, &linegap);
		baseline = (int)(ascent * scale);
		int advance, lsb, x0, y0, x1, y1;
		//
		float shiftX = 0.0f;
		float shiftY = 0.0f;
		//
		stbtt_GetCodepointHMetrics(&m_fontHandle, _char.charCode, &advance, &lsb);
		stbtt_GetCodepointBitmapBoxSubpixel(&m_fontHandle, _char.charCode, scale, scale, shiftX, shiftY, &x0, &y0, &x1, &y1);
		stbtt_MakeCodepointBitmapSubpixel(&m_fontHandle, 
			&m_outputData[0],
			x1 - x0, 
			y1 - y0, 
			x1 - x0, // screen width ( stride )
			scale, scale, 
			shiftX, shiftY, // shift x, shift y 
			_char.charCode);
		CharactorInfo c;
		c.bearingX = x0;
		c.bearingY = -y0;
		c.width = x1 - x0;
		c.height = y1 - y0;
		Nix::Rect<uint16_t> rc;
		if (c.height > 0 && c.width > 0) {
			bool rst = this->m_texturePacker->insert(m_outputData, c.width * c.height, c.width, c.height, rc);
			c.x = rc.origin.x;
			c.y = rc.origin.y;
			m_mappingTable[_char.key] = c;
			return m_mappingTable[_char.key];
		}
		else
		{
			return c;
		}
	}
}