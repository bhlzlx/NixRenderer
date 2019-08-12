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

	UIDrawData* UIRenderer::build(const PlainTextDraw& _draw, UIDrawData* _oldDrawData, Nix::Rect<float>& _rc )
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
		m_textureManager.getFontTextureManger()->getLineHeight(_draw.fontId, _draw.fontSize, textHeight,baseLine);

		float baseX = 0;
		float baseY = baseLine;
		//
		// ��Ϊ�����Ժ򻹻���һ�β��ֶ��룬���Լ����ı�ռ�������ʱ��ԭ��������Ϊ��0�� 0��
		
		UIVertex* vtxBegin;
		//UIVertex* vtxEnd;

		UIVertex* vtx = (UIVertex*)allocation.ptr;
		vtxBegin = vtx;
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
			vtx[1].y = baseY - (charInfo.height - charInfo.bearingY);
			vtx[0].x = vtx[1].x;
			vtx[0].y = vtx[1].y + charInfo.height;
			vtx[3].x = vtx[0].x + charInfo.width;
			vtx[3].y = vtx[0].y;
			vtx[2].x = vtx[3].x;
			vtx[2].y = vtx[1].y;
			// configure [u,v]
			vtx[0].u = (float)charInfo.x / (UITextureSize-1);
			vtx[0].v = (float)charInfo.y / (UITextureSize - 1);
			vtx[1].u = vtx[0].u;
			vtx[1].v = (float)(charInfo.y + charInfo.height) / (UITextureSize - 1);
			vtx[3].u = (float)(charInfo.x + charInfo.width) / (UITextureSize - 1);
			vtx[3].v = vtx[0].v;
			vtx[2].u = vtx[3].u;
			vtx[2].v = vtx[1].v;
			// configure layer
			vtx[0].layer = vtx[1].layer = vtx[2].layer = vtx[3].layer = charInfo.layer;
			//
			vtx[0].color = _draw.colorMask;
			vtx[1].color = _draw.colorMask;
			vtx[2].color = _draw.colorMask;
			vtx[3].color = _draw.colorMask;
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
		for (uint32_t i = 0; i < _draw.length * 4; ++i) {
			vtxBegin->x += alignedRc.origin.x;
			vtxBegin->y += alignedRc.origin.y + baseLine;
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
			void getLineH( Nix::UITextureManager* _tm, uint16_t _fontID, uint16_t _fontSize, float& _baseLine, float& _height) {
				ULine key;
				key.fontId = _fontID;
				key.fontSize = _fontSize;
				auto it = lineHTable.find(key.lineKey);
				if ( it == lineHTable.end()) {
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
			if (line->rc.size.width + charInfo.adv > _draw.rect.size.width ) {
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
			//if (ch.code > 256) {
//				vtx[1].y = -2;
	//		}
		//	else {
				vtx[1].y = -(charInfo.height - charInfo.bearingY);
			//}
			vtx[0].x = vtx[1].x;
			vtx[0].y = vtx[1].y + charInfo.height;
			vtx[3].x = vtx[0].x + charInfo.width;
			vtx[3].y = vtx[0].y;
			vtx[2].x = vtx[3].x;
			vtx[2].y = vtx[1].y;
			// configure [u,v]
			vtx[0].u = (float)charInfo.x / (UITextureSize - 1);
			vtx[0].v = (float)charInfo.y / (UITextureSize - 1);
			vtx[1].u = vtx[0].u;
			vtx[1].v = (float)(charInfo.y + charInfo.height) / (UITextureSize - 1);
			vtx[3].u = (float)(charInfo.x + charInfo.width) / (UITextureSize - 1);
			vtx[3].v = vtx[0].v;
			vtx[2].u = vtx[3].u;
			vtx[2].v = vtx[1].v;
			// configure layer
			vtx[0].layer = vtx[1].layer = vtx[2].layer = vtx[3].layer = charInfo.layer;
			//
			vtx[0].color = ch.color;
			vtx[1].color = ch.color;
			vtx[2].color = ch.color;
			vtx[3].color = ch.color;
			vtx += 4;
			line->vertexCount += 4;
		}
		contentRect.size.height += line->rc.size.height;
		//
		auto alignedContentRc = Nix::alignRect<float>( contentRect, _draw.rect, _draw.calign, UIHoriAlign::UIAlignHoriMid );
		// do the alignment
		for (auto& line : lines) {
			auto alignedRc = Nix::alignRect<float>(line.rc, alignedContentRc, _draw.valign, _draw.halign);
			float offsetx = alignedRc.origin.x;
			float offsety = alignedRc.origin.y + line.baseLine;
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
		if ( _oldDrawData ) {
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
		//
		UIVertex* vtx = (UIVertex*)drawData->vertexBufferAllocation.ptr;
		for (uint32_t i = 0; i < _count; ++i) {
			vtx[0].x = _draws->rect.origin.x;
			vtx[0].y = _draws->rect.origin.y + _draws->rect.size.height;
			vtx[0].u = _draws->uv[0].x;
			vtx[0].v = _draws->uv[0].y;
			
			vtx[2].x = _draws->rect.origin.x + _draws->rect.size.width;
			vtx[2].y = _draws->rect.origin.y;
			vtx[2].u = _draws->uv[2].x;
			vtx[2].v = _draws->uv[2].y;

			vtx[1].x = vtx[0].x;
			vtx[1].y = vtx[2].y;
			vtx[1].u = _draws->uv[1].x;
			vtx[1].v = _draws->uv[1].y;

			vtx[3].x = vtx[2].x;
			vtx[3].y = vtx[0].y;
			vtx[3].u = _draws->uv[3].x;
			vtx[3].v = _draws->uv[3].y;
			//
			vtx[0].color = vtx[1].color = vtx[2].color = vtx[3].color = _draws->color;
			vtx[0].layer = vtx[1].layer = vtx[2].layer = vtx[3].layer = _draws->layer;
			//
			vtx += 4;
		}
		return drawData;
	}

	UIDrawData* UIRenderer::copyDrawData(UIDrawData* _drawData)
	{
		UIDrawData* drawData = m_drawDataPool.newElement();
		drawData->type = _drawData->type;
		drawData->primitiveCapacity = _drawData->primitiveCapacity;
		drawData->primitiveCount = _drawData->primitiveCount;
		if (_drawData->type == UITriangle) {
			drawData->vertexBufferAllocation = m_vertexMemoryHeap.allocateTriangles(_drawData->primitiveCapacity);
			memcpy( drawData->vertexBufferAllocation.ptr, _drawData->vertexBufferAllocation.ptr, drawData->primitiveCount * 3 * sizeof(UIVertex));
		}
		else {
			drawData->vertexBufferAllocation = m_vertexMemoryHeap.allocateRects(_drawData->primitiveCapacity);
			memcpy(drawData->vertexBufferAllocation.ptr, _drawData->vertexBufferAllocation.ptr, drawData->primitiveCount * 4 * sizeof(UIVertex));
		}
		return drawData;
	}

	void UIRenderer::transformDrawData(UIDrawData* _draw, float _offsetX, float _offsetY, UIDrawData* _to)
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


