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

	UIDrawData* UIRenderer::build(const TextDraw& _draw, UIDrawData* _oldDrawData)
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
		m_fontTexManager.getLineHeight(_draw.fontId, _draw.fontSize, textHeight,baseLine);

		float baseX = 0;
		float baseY = baseLine;
		//
		// 因为我们稍候还会做一次布局对齐，所以计算文本占用区域的时候，原点我们设为（0， 0）
		
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
			auto& charInfo = m_fontTexManager.getCharactor(ck);
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
			vtx[0].u = (float)charInfo.x / TexturePackerWidth;
			vtx[0].v = (float)charInfo.y / TexturePackerHeight;
			vtx[1].u = vtx[0].u;
			vtx[1].v = (float)(charInfo.y + charInfo.height) / TexturePackerHeight;
			vtx[3].u = (float)(charInfo.x + charInfo.width) / TexturePackerWidth;
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
		Nix::Rect<int16_t> textRect = {
			{0 , 0},
			{(int16_t)baseX, (int16_t)textHeight}
		};
		Nix::Rect<int16_t> alignedRc = alignRect<int16_t>(textRect, _draw.rect, _draw.valign, _draw.halign);
		for (uint32_t i = 0; i < _draw.length * 4; ++i) {
			vtxBegin->x += alignedRc.origin.x;
			vtxBegin->y += alignedRc.origin.y;
			++vtxBegin;
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
}
