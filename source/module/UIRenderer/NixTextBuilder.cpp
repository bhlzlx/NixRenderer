#pragma once
#include "NixTextBuilder.h"
#include "NixUIRenderer.h"
#include "NixUIDefine.h"
#include "NixFontTextureManager.h"
#include "NixUIUtils.h"

namespace Nix {
	/* *****************************************************
	*  |----------------------------------------|
	*  |                                        |
	*  |----------------------------------------|
	*  |----------------------------------------|
	******************************************************/

	UIDrawData* UIRenderer::build(const PlainTextDraw& _draw, UIDrawData* _oldDrawData, Nix::Rect<float>& _rc)
	{
		UIDrawData* drawData = nullptr;
		if (_oldDrawData) {
			if (_oldDrawData->primitiveCount < _draw.length || _oldDrawData->type != UITopologyType::UIRectangle) {
				m_vertexMemoryHeap.free(_oldDrawData->vertexBufferAllocation);
				_oldDrawData->vertexBufferAllocation = m_vertexMemoryHeap.allocateRects(_draw.length);
				_oldDrawData->primitiveCapacity = _oldDrawData->primitiveCount = _draw.length;
			}
			drawData = _oldDrawData;
			_oldDrawData->primitiveCount = _draw.length;
		}
		else {
			drawData = m_drawDataPool.newElement();
			drawData->vertexBufferAllocation = m_vertexMemoryHeap.allocateRects(_draw.length);
			drawData->primitiveCapacity = drawData->primitiveCount = _draw.length;
		}
		//
		DrawDataMemoryHeap::Allocation& allocation = drawData->vertexBufferAllocation;
		//
		//float textWidth;
		float baseLine;
		float textHeight = 0;
		m_textureManager.getFontTextureManger()->getLineHeight(_draw.fontId, _draw.fontSize, textHeight, baseLine);

		float baseX = 0;
		float baseY = baseLine;
		//

		UIVertex* vtxBegin;
		//UIVertex* vtxEnd;

		UIVertex* vtx = (UIVertex*)allocation.ptr;
		vtxBegin = vtx;

		drawData->uniformData.clear();
		//
		for (uint32_t charIdx = 0; charIdx < _draw.length; ++charIdx) {
			Nix::CharKey ck;
			ck.charCode = _draw.text[charIdx];
			ck.fontId = _draw.fontId;
			ck.size = _draw.fontSize;
			auto& charInfo = m_textureManager.getFontTextureManger()->getCharactor(ck);
			/* ----------
			*	0 -- 3
			*	| \  |
			*	|  \ |
			*	1 -- 2
			* --------- */
			// configure [x,y] positions
			vtx[1].x = baseX + charInfo.bearingX;
			vtx[1].y = charInfo.height - charInfo.bearingY;

			vtx[0].x = vtx[1].x;
			vtx[0].y = charInfo.height - charInfo.bearingY - charInfo.height;
			vtx[3].x = vtx[0].x + charInfo.width;
			vtx[3].y = vtx[0].y;
			vtx[2].x = vtx[3].x;
			vtx[2].y = vtx[1].y;
			// configure [u,v]
			vtx[0].u = (float)charInfo.x / (UITextureSize);
			vtx[0].v = (float)charInfo.y / (UITextureSize);
			vtx[1].u = vtx[0].u;
			vtx[1].v = (float)(charInfo.y + charInfo.height) / (UITextureSize);
			vtx[3].u = (float)(charInfo.x + charInfo.width) / (UITextureSize);
			vtx[3].v = vtx[0].v;
			vtx[2].u = vtx[3].u;
			vtx[2].v = vtx[1].v;
			// configure layer
			UIUniformElement unif;
			unif.color = _draw.colorMask;
			unif.layer = charInfo.layer;
			drawData->uniformData.push_back(unif);
			vtx += 4;
			baseX += charInfo.adv;// charInfo.bearingX + charInfo.width;
		}
		drawData->type = UIRectangle;
		//
		Nix::Rect<float> textRect = {
			{0 , 0},
			{baseX, textHeight}
		};
		Nix::Rect<float> alignedRc = alignRect<float>(textRect, _draw.rect, _draw.valign, _draw.halign);
		float offsety = alignedRc.origin.y + textHeight - baseLine;
		for (uint32_t i = 0; i < _draw.length * 4; ++i) {
			vtxBegin->x += alignedRc.origin.x;
			vtxBegin->y += offsety;
			++vtxBegin;
		}
		_rc = alignedRc;
		return drawData;
	}

	UIDrawData* UIRenderer::build(const RichTextDraw& _draw, bool _autoWrap, UIDrawData* _oldDrawData, Nix::Rect<float>& _rc)
	{
		struct LineHeightGetter {
			typedef union {
				struct {
					uint16_t fontId;
					uint16_t fontSize;
				};
				uint32_t lineKey;
			} ULine;
			struct LineH {
				float height;
				float baseLine;
			};
			std::map<uint32_t, LineH> lineHTable;
			void getLineH(Nix::UITextureManager* _tm, uint16_t _fontID, uint16_t _fontSize, float& _baseLine, float& _height) {
				ULine key;
				key.fontId = _fontID;
				key.fontSize = _fontSize;
				auto it = lineHTable.find(key.lineKey);
				if (it == lineHTable.end()) {
					_tm->getFontTextureManger()->getLineHeight(_fontID, _fontSize, _height, _baseLine);
				}
				else {
					_baseLine = it->second.baseLine;
					_height = it->second.height;
				}
			}
		};
		//
		LineHeightGetter lineHGetter;
		//
		UIDrawData* drawData = nullptr;
		if (_oldDrawData) {
			if (_oldDrawData->primitiveCount < _draw.vecChar.size() || _oldDrawData->type != UITopologyType::UIRectangle) {
				m_vertexMemoryHeap.free(_oldDrawData->vertexBufferAllocation);
				_oldDrawData->vertexBufferAllocation = m_vertexMemoryHeap.allocateRects(_draw.vecChar.size());
				_oldDrawData->primitiveCapacity = _oldDrawData->primitiveCount = _draw.vecChar.size();
			}
			drawData = _oldDrawData;
			_oldDrawData->primitiveCount = _draw.vecChar.size();
		}
		else {
			drawData = m_drawDataPool.newElement();
			drawData->vertexBufferAllocation = m_vertexMemoryHeap.allocateRects(_draw.vecChar.size());
			drawData->primitiveCapacity = drawData->primitiveCount = _draw.vecChar.size();
		}
		drawData->uniformData.clear();
		//
		DrawDataMemoryHeap::Allocation& allocation = drawData->vertexBufferAllocation;
		UIVertex* vtx = (UIVertex*)allocation.ptr;
		//
		struct Line {
			Nix::Rect<float>				rc;		  //>! the bounding box of the line
			float							baseLine; //>! different font id/size have different baseline value, we chose the maximum base line
			UIVertex*						vertices; //>! the vertex.x is the position relative to the left of base line
			uint32_t						vertexCount;
			Line() {
				rc.origin.x = rc.origin.y = 0;
				rc.size.width = rc.size.height = 0;
				vertexCount = 0;
			}
		};

		Nix::Rect<float> contentRect;
		contentRect.origin = { 0, 0 };
		contentRect.size = { _draw.rect.size.width, 0 };

		std::vector<Line> lines;
		//
		//float currentLineWidth = 0;
		float currentLineHeight = 0;
		//
		lines.resize(lines.size() + 1);
		Line* line = &lines[0];
		line->vertices = vtx;
		for (auto& ch : _draw.vecChar) {
			//
			float xPos = line->rc.size.width;
			//
			Nix::CharKey ck;
			ck.charCode = ch.code;
			ck.fontId = ch.font;
			ck.size = ch.size;
			auto& charInfo = m_textureManager.getFontTextureManger()->getCharactor(ck);
			// ====================== update line information ======================
			if (line->rc.size.width + charInfo.adv > _draw.rect.size.width) {
				// new line
				size_t index = lines.size() - 1;
				lines.resize(lines.size() + 1);
				auto& prevLine = lines[index];
				contentRect.size.height += prevLine.rc.size.height;
				++index;
				auto& currentLine = lines[index];
				line = &currentLine;
				line->vertices = vtx;
				line->rc.origin.x = 0;
				line->rc.origin.y = prevLine.rc.origin.y + prevLine.rc.size.height;
				line->baseLine = 0;
				line->rc.size = { 0, 0 };
				xPos = 0.0f;
			}
			line->rc.size.width += charInfo.adv;
			float bl;// baseline
			float lh; // line height
			lineHGetter.getLineH(&m_textureManager, ck.fontId, ck.size, bl, lh);
			if (lh > line->rc.size.height)
				line->rc.size.height = lh;
			if (bl > line->baseLine)
				line->baseLine = bl;
			// ====================== update vertices information ======================
			/* ----------
			*	0 -- 3
			*	| \  |
			*	|  \ |
			*	1 -- 2
			* --------- */
			// configure [x,y] positions
			vtx[1].x = xPos + charInfo.bearingX;
			vtx[1].y = charInfo.height - charInfo.bearingY;

			vtx[0].x = vtx[1].x;
			vtx[0].y = charInfo.height - charInfo.bearingY - charInfo.height;
			vtx[3].x = vtx[0].x + charInfo.width;
			vtx[3].y = vtx[0].y;
			vtx[2].x = vtx[3].x;
			vtx[2].y = vtx[1].y;
			// configure [u,v]
			vtx[0].u = (float)charInfo.x / (UITextureSize);
			vtx[0].v = (float)charInfo.y / (UITextureSize);
			vtx[1].u = vtx[0].u;
			vtx[1].v = (float)(charInfo.y + charInfo.height) / (UITextureSize);
			vtx[3].u = (float)(charInfo.x + charInfo.width) / (UITextureSize);
			vtx[3].v = vtx[0].v;
			vtx[2].u = vtx[3].u;
			vtx[2].v = vtx[1].v;
			// configure layer
			UIUniformElement unif;
			unif.color = ch.color;
			unif.layer = charInfo.layer;
			drawData->uniformData.push_back(unif);
			line->vertexCount += 4;
			vtx += 4;
		}
		contentRect.size.height += line->rc.size.height;
		//
		auto alignedContentRc = Nix::alignRect<float>(contentRect, _draw.rect, _draw.calign, UIHoriAlign::UIAlignHoriMid);
		// do the alignment
		for (auto& line : lines) {
			auto alignedRc = Nix::alignRect<float>(line.rc, alignedContentRc, _draw.valign, _draw.halign);
			float offsetx = alignedRc.origin.x;
			float offsety = alignedRc.origin.y + line.rc.size.height - line.baseLine;
			// apply the aligned x/y to vertices
			for (uint32_t i = 0; i < line.vertexCount; ++i) {
				auto& vertex = line.vertices[i];
				vertex.x += offsetx;
				vertex.y += offsety;
			}
		}
		drawData->type = UITopologyType::UIRectangle;
		_rc = alignedContentRc;
		return drawData;
	}

	UIDrawData* UIRenderer::build(const ImageDraw* _draws, uint32_t _count, UIDrawData* _oldDrawData)
	{
		UIDrawData* drawData = nullptr;
		if (_oldDrawData) {
			if (_oldDrawData->type == UIRectangle && _oldDrawData->primitiveCapacity >= _count) {
				drawData = _oldDrawData;
				drawData->primitiveCount = _count;
			}
			else {
				this->destroyDrawData(_oldDrawData);
			}
		}
		if (!drawData) {
			drawData = m_drawDataPool.newElement();
			drawData->primitiveCapacity = drawData->primitiveCount = _count;
			drawData->type = UIRectangle;
			drawData->vertexBufferAllocation = m_vertexMemoryHeap.allocateRects(_count);
		}
		drawData->uniformData.clear();
		//
		UIVertex* vtx = (UIVertex*)drawData->vertexBufferAllocation.ptr;
		for (uint32_t i = 0; i < _count; ++i) {
			vtx[0].x = _draws[i].rect.origin.x;
			vtx[0].y = _draws[i].rect.origin.y;
			vtx[0].u = _draws[i].uv[0].x;
			vtx[0].v = _draws[i].uv[0].y;
			vtx[2].x = _draws[i].rect.origin.x + _draws[i].rect.size.width;
			vtx[2].y = _draws[i].rect.origin.y + _draws[i].rect.size.height;
			vtx[2].u = _draws[i].uv[2].x;
			vtx[2].v = _draws[i].uv[2].y;

			vtx[1].x = vtx[0].x;
			vtx[1].y = vtx[2].y;
			vtx[1].u = _draws[i].uv[1].x;
			vtx[1].v = _draws[i].uv[1].y;

			vtx[3].x = vtx[2].x;
			vtx[3].y = vtx[0].y;
			vtx[3].u = _draws[i].uv[3].x;
			vtx[3].v = _draws[i].uv[3].y;
			//
			vtx += 4;
			UIUniformElement unif;
			unif.color = _draws[i].color;
			unif.layer = _draws[i].layer;
			drawData->uniformData.push_back(unif);
		}
		return drawData;
	}

	//UIDrawData* UIRenderer::build(uint32_t _numTri, UIDrawData* _oldDrawData)
	//{
	//	UIDrawData* drawData = nullptr;
	//	if (_oldDrawData) {
	//		if (_oldDrawData->type == UITriangle && _oldDrawData->primitiveCapacity >= _numTri) {
	//			drawData = _oldDrawData;
	//			drawData->primitiveCount = _numTri;
	//		}
	//		else {
	//			this->destroyDrawData(_oldDrawData);
	//		}
	//	}
	//	if (!drawData) {
	//		drawData = m_drawDataPool.newElement();
	//		drawData->primitiveCapacity = drawData->primitiveCount = _numTri;
	//		drawData->type = UIRectangle;
	//		drawData->vertexBufferAllocation = m_vertexMemoryHeap.allocateTriangles(_numTri);
	//	}
	//	drawData->type = UITriangle;
	//	return drawData;
	//}

	UIDrawData* UIRenderer::copyDrawData(UIDrawData* _drawData)
	{
		UIDrawData* drawData = m_drawDataPool.newElement();
		drawData->type = _drawData->type;
		drawData->primitiveCapacity = _drawData->primitiveCapacity;
		drawData->primitiveCount = _drawData->primitiveCount;
		if (_drawData->type == UITriangle) {
			drawData->vertexBufferAllocation = m_vertexMemoryHeap.allocateTriangles(_drawData->primitiveCapacity);
			memcpy(drawData->vertexBufferAllocation.ptr, _drawData->vertexBufferAllocation.ptr, drawData->primitiveCount * 3 * sizeof(UIVertex));
		}
		else {
			drawData->vertexBufferAllocation = m_vertexMemoryHeap.allocateRects(_drawData->primitiveCapacity);
			memcpy(drawData->vertexBufferAllocation.ptr, _drawData->vertexBufferAllocation.ptr, drawData->primitiveCount * 4 * sizeof(UIVertex));
		}
		return drawData;
	}

	void UIRenderer::translateDrawData(UIDrawData* _draw, float _offsetX, float _offsetY, UIDrawData* _to)
	{
		assert(_draw != _to);
		assert(_draw->primitiveCount == _to->primitiveCount);
		//
		uint32_t count = _draw->type == UIRectangle ? _draw->primitiveCount * 4 : _draw->primitiveCount * 3;
		UIVertex* vtxSrc = (UIVertex*)_draw->vertexBufferAllocation.ptr;
		UIVertex* vtxDst = (UIVertex*)_to->vertexBufferAllocation.ptr;
		for (uint32_t i = 0; i < count; ++i) {
			vtxDst->x = vtxSrc->x + _offsetX;
			vtxDst->y = vtxSrc->y + _offsetY;
			++vtxDst;
			++vtxSrc;
		}
	}

	void UIRenderer::rotateDrawData(UIDrawData* _draw, const Nix::Point<float>& _anchor, float _angle, UIDrawData* _to)
	{
		auto funRotate = [](const Nix::Point<float>& _point, const Nix::Point<float>& _anchor, float _angle) -> Nix::Point<float> {
			Nix::Point<float> ret;
			float arc = _angle / 180.0f * 3.1415926f;
			ret.x = (_point.x - _anchor.x) * cosf(-arc) - (_point.y - _anchor.y) * sinf(-arc) + _anchor.x;
			ret.y = (_point.x - _anchor.x) * sinf(-arc) + (_point.y - _anchor.y) * cosf(-arc) + _anchor.y;
			return ret;
		};
		assert(_draw->primitiveCount == _to->primitiveCount);
		//
		uint32_t count = _draw->type == UIRectangle ? _draw->primitiveCount * 4 : _draw->primitiveCount * 3;
		UIVertex* vtxSrc = (UIVertex*)_draw->vertexBufferAllocation.ptr;
		UIVertex* vtxDst = (UIVertex*)_to->vertexBufferAllocation.ptr;
		for (uint32_t i = 0; i < count; ++i) {
			const Nix::Point<float> pt = { vtxSrc->x, vtxSrc->y };
			auto rst = funRotate(pt, _anchor, _angle);
			vtxDst->x = rst.x;
			vtxDst->y = rst.y;
			++vtxDst;
			++vtxSrc;
		}
	}

	void UIRenderer::transformDrawData(UIDrawData* _draw, const Nix::Point<float>& _anchor, float _angle, float _scale, UIDrawData* _to)
	{
		auto funTransform = [](const Nix::Point<float>& _point, const Nix::Point<float>& _anchor, float _angle, float _scale) -> Nix::Point<float> {
			Nix::Point<float> ret;
			float arc = _angle / 180.0f * 3.1415926f;
			ret.x = _scale*((_point.x - _anchor.x) * cosf(-arc) - (_point.y - _anchor.y) * sinf(-arc)) + _anchor.x;
			ret.y = _scale*((_point.x - _anchor.x) * sinf(-arc) + (_point.y - _anchor.y) * cosf(-arc)) + _anchor.y;
			return ret;
		};
		assert(_draw->primitiveCount == _to->primitiveCount);
		//
		uint32_t count = _draw->type == UIRectangle ? _draw->primitiveCount * 4 : _draw->primitiveCount * 3;
		UIVertex* vtxSrc = (UIVertex*)_draw->vertexBufferAllocation.ptr;
		UIVertex* vtxDst = (UIVertex*)_to->vertexBufferAllocation.ptr;
		for (uint32_t i = 0; i < count; ++i) {
			const Nix::Point<float> pt = { vtxSrc->x, vtxSrc->y };
			auto rst = funTransform(pt, _anchor, _angle, _scale);
			vtxDst->x = rst.x;
			vtxDst->y = rst.y;
			++vtxDst;
			++vtxSrc;
		}
	}

	void UIRenderer::scissorDrawData(UIDrawData* _draw, const Nix::Scissor& _scissor, UIDrawData* _output)
	{
		assert(_draw->type == UIRectangle);
		assert(_output->type == _draw->type);
		assert(_output->primitiveCapacity >= _draw->primitiveCount);
		//
		UIVertex *output = (UIVertex*)_output->vertexBufferAllocation.ptr;
		UIVertex *input = (UIVertex*)_draw->vertexBufferAllocation.ptr;
		uint32_t primitiveCount = 0;
		for (uint32_t i = 0; i < _draw->primitiveCount; ++i) {
			VertexClipFlags flag = clipRect(input, _scissor, output);
			if (flag != VertexClipFlagBits::AllClipped) {
				output += 4;
				++primitiveCount;
			}
			input += 4;
		}
		_output->primitiveCount = primitiveCount;
	}
}
