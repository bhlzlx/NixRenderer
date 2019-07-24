#include "NixUIRenderer.h"
#include <nix/io/archieve.h>

namespace Nix {

	static const uint16_t TexturePackerWidth = 512;
	static const uint16_t TexturePackerHeight = 512;
	static const uint16_t TexturePackerDepth = 2;

	inline bool UIRenderer::initialize(IContext* _context, IArchieve* _archieve) {
		m_packerLibrary = OpenLibrary("TexturePacker.dll");
		if (!m_packerLibrary) {
			return false;
		}
		m_createPacker = (PFN_CREATE_TEXTURE_PACKER)GetExportAddress(m_packerLibrary, "CreateTexturePacker");
		if (!m_createPacker) {
			return false;
		}
		Nix::Size3D<uint16_t> texSize = { TexturePackerWidth, TexturePackerHeight, TexturePackerDepth };
		bool rst = m_fontTexManager.initialize(_context, _archieve, texSize, m_createPacker);
		if (!rst) {
			return false;
		}
		return true;
	}
	PrebuildDrawData* UIRenderer::build(const TextDraw& _draw, PrebuildDrawData* _oldDrawData)
	{
		PrebuildDrawData* drawData = nullptr;
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
			drawData = m_prebuilDrawDataPool.newElement();
			drawData->vertexBufferAllocation = m_vertexMemoryHeap.allocateRects(_draw.length);
			drawData->primitiveCapacity = _oldDrawData->primitiveCount = _draw.length;
		}
		//
		PrebuildBufferMemoryHeap::Allocation& allocation = drawData->vertexBufferAllocation;
		//
		int x = _draw.original.x;
		int y = _draw.original.y;
		//
		UIVertex* vtx = (UIVertex*)allocation.ptr;
		for (uint32_t charIdx = 0; charIdx < _draw.length; ++charIdx) {
			Nix::CharKey c;
			c.charCode = _draw.text[charIdx];
			c.fontId = _draw.fontId;
			c.size = _draw.fontSize;
			auto& charInfo = m_fontTexManager.getCharactor(c);
			/* ----------
			*	0 -- 3
			*	| \  | 
			*	|  \ |
			*	1 -- 2
			* --------- */
			// configure [x,y] positions
			vtx[1].x = x + charInfo.bearingX;
			vtx[1].y = y + (charInfo.height - charInfo.bearingY);
			vtx[0].x = vtx[1].x;
			vtx[0].y = vtx[1].y + charInfo.height;
			vtx[3].x = vtx[1].x + charInfo.width;
			vtx[3].y = vtx[1].y;
			vtx[2].x = vtx[3].x;
			vtx[2].y = vtx[0].y;
			// configure [u,v]
			vtx[0].u = (float)charInfo.x / TexturePackerWidth - 1;
			vtx[0].v = (float)charInfo.y / TexturePackerHeight - 1;
			vtx[1].u = vtx[0].u;
			vtx[1].v = (float)(charInfo.y + charInfo.height) / TexturePackerHeight - 1;
			vtx[3].u = (float)(charInfo.x + charInfo.width) / TexturePackerWidth - 1;
			vtx[3].v = vtx[1].v;
			vtx[2].u = vtx[3].u;
			vtx[2].v = vtx[1].v;
			// configure layer
			vtx[0].layer = vtx[1].layer = vtx[2].layer = vtx[3].layer = charInfo.layer;
			//
			vtx += 4;
			x += charInfo.bearingX;
		}
		drawData->drawState.alpha = _draw.alpha;
		memcpy(&drawData->drawState.scissor, &_draw.scissor, sizeof(_draw.scissor));
		//
		return drawData;
	}

	// -------------------------------------------------------------------------------------

	void UIMeshManager::initialize()
	{
	}

	void UIMeshManager::beginFrame(uint32_t _flightIndex)
	{
		this->m_flightIndex = _flightIndex;
		this->m_currentBufferIndex = 0;
		this->m_currentBufferOffset = 0;
		this->m_vecDrawComamnds.clear();
	}

	void UIMeshManager::pushDrawData( const PrebuildDrawData* _drawData )
	{
		uint32_t verticesNeed = 0;
		if (_drawData->type == UITopologyType::UIRectangle) {
			verticesNeed = 4 * _drawData->primitiveCount;
		}
		else {
			verticesNeed = 3 * _drawData->primitiveCount;
		}
		if (verticesNeed > MaxVertexCount - m_currentBufferOffset) {
			++m_currentBufferIndex;
			if( m_currentBufferIndex >= m_vertexBufferPM)
		}
		DrawCommand* prevDrawCommand = m_vecDrawComamnds.size() ? &m_vecDrawComamnds.back() : nullptr;
		//
		if (prevDrawCommand && prevDrawCommand->state == _drawData->drawState) {
		}
	}

	void UIMeshManager::endFrame()
	{
	}

}