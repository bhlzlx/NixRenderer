#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#define NIX_JP_IMPLEMENTATION
#include <NixJpDecl.h>
#include <NixRenderer.h>
#include "NixUIRenderer.h"
#include <nix/io/archieve.h>
#include "TexturePacker/TexturePacker.h"

#ifdef _WIN32
#include <Windows.h>
#define OpenLibrary( name ) (void*)::LoadLibraryA(name)
#define CloseLibrary( library ) ::FreeLibrary((HMODULE)library)
#define GetExportAddress( libray, function ) ::GetProcAddress( (HMODULE)libray, function )
#else
#include <dlfcn.h>
#define OpenLibrary( name ) dlopen(name , RTLD_NOW | RTLD_LOCAL)
#define CloseLibrary( library ) dlclose((void*)library)
#define GetExportAddress( libray, function ) dlsym( (void*)libray, function )
#endif

namespace Nix {

	static const uint16_t TexturePackerWidth = 512;
	static const uint16_t TexturePackerHeight = 512;
	static const uint16_t TexturePackerDepth = 2;

	bool UIRenderer::initialize(IContext* _context, IArchieve* _archieve) {
		m_packerLibrary = OpenLibrary("TexturePacker.dll");
		if (!m_packerLibrary) {
			return false;
		}
		m_createPacker = (PFN_CREATE_TEXTURE_PACKER)GetExportAddress(m_packerLibrary, "CreateTexturePacker");
		if (!m_createPacker) {
			return false;
		}
		m_context = _context;
		MaterialDescription mtl;
		strcpy(mtl.vertexShader, "ui/ui.vert");
		strcpy(mtl.fragmentShader, "ui/ui.frag");
		mtl.argumentCount = 1; {
			mtl.argumentLayouts[0].descriptorCount = 1;
			mtl.argumentLayouts[0].index = 0; {
				mtl.argumentLayouts[0].descriptors[0].type = Nix::SDT_Sampler;
				mtl.argumentLayouts[0].descriptors[0].binding = 0;
				mtl.argumentLayouts[0].descriptors[0].shaderStage = Nix::FragmentShader;
				strcpy(mtl.argumentLayouts[0].descriptors[0].name, "UiTexArray");
			}			
		}
		mtl.pologonMode = Nix::PMFill; {
			mtl.renderState.cullMode = Nix::CullNone;
			mtl.renderState.blendState.enable = true;
			mtl.renderState.blendState.srcFactor = SourceAlpha;
			mtl.renderState.blendState.dstFactor = InvertSourceAlpha;
			mtl.renderState.depthState.testable = false;
			mtl.renderState.depthState.writable = false;
			mtl.renderState.windingMode = Nix::CounterClockwise;
			mtl.renderState.scissorEnable = true;
			mtl.renderState.stencilState.enable = false;
			mtl.renderState.writeMask = 0xff;
		}
		mtl.topologyMode = Nix::TMTriangleList;
		//
		mtl.vertexLayout.attributeCount = 3;
		mtl.vertexLayout.bufferCount = 1;
		mtl.vertexLayout.buffers[0].stride = sizeof(UIVertex);
		mtl.vertexLayout.buffers[0].instanceMode = 0;
		//
		strcpy(mtl.vertexLayout.attributes[0].name, "position");
		mtl.vertexLayout.attributes[0].offset = 0;
		mtl.vertexLayout.attributes[0].bufferIndex = 0;
		mtl.vertexLayout.attributes[0].type = Nix::VertexTypeFloat2;
		
		strcpy(mtl.vertexLayout.attributes[1].name, "uv");
		mtl.vertexLayout.attributes[1].offset = 8;
		mtl.vertexLayout.attributes[1].bufferIndex = 0;
		mtl.vertexLayout.attributes[1].type = Nix::VertexTypeFloat3;
		
		strcpy(mtl.vertexLayout.attributes[2].name, "colorMask");
		mtl.vertexLayout.attributes[2].offset = 20;
		mtl.vertexLayout.attributes[2].bufferIndex = 0;
		mtl.vertexLayout.attributes[2].type = Nix::VertexTypeUint;
		
		m_material = _context->createMaterial(mtl);
		if (!m_material) {
			return false;
		}
		Nix::TextReader rpReader;
		rpReader.openFile(_archieve, "renderpass/swapchain.json");
		RenderPassDescription rpDesc;
		rpDesc.parse(rpReader.getText());
		rpDesc.colors[0].format = m_context->swapchainColorFormat();
		rpDesc.depthStencil.format = m_context->getDriver()->selectDepthFormat(true);
		m_pipeline = m_material->createPipeline(rpDesc);
		if (!m_pipeline) {
			return false;
		}
		Nix::TextureDescription texDesc;
		texDesc.depth = 4;
		texDesc.format = NixRGBA8888_UNORM;
		texDesc.width = texDesc.height = 512;
		texDesc.mipmapLevel = 1;
		texDesc.type = Texture2DArray;
		m_uiTexArray = m_context->createTexture(texDesc);
		m_argument = m_material->createArgument(0);
		if (!m_argument || !m_uiTexArray) {
			return false;
		}
		for (uint32_t i = 0; i<MaxFlightCount; ++i) {
			m_renderables[i].push_back(m_material->createRenderable());
			m_meshBuffers[i].resize(1);
			m_meshBuffers[i].back().initialize(_context, m_renderables[i][0], MaxVertexCount);
		}
		bool rst = m_fontTexManager.initialize(_context, _archieve, m_uiTexArray, m_createPacker, 2);
		if (!rst) {
			return false;
		}
		SamplerState ss;
		m_argument->setSampler(0, ss, m_uiTexArray);
		return true;
	}

	uint32_t UIRenderer::addFont(const char * _filepath)
	{
		return m_fontTexManager.addFont(_filepath);
	}

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
			drawData = m_prebuilDrawDataPool.newElement();
			drawData->vertexBufferAllocation = m_vertexMemoryHeap.allocateRects(_draw.length);
			drawData->primitiveCapacity = drawData->primitiveCount = _draw.length;
		}
		//
		PrebuildBufferMemoryHeap::Allocation& allocation = drawData->vertexBufferAllocation;
		//
		int x = _draw.origin.x;
		int y = _draw.origin.y;
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
			vtx[1].y = y - (charInfo.height - charInfo.bearingY);
			vtx[0].x = vtx[1].x;
			vtx[0].y = vtx[1].y + charInfo.height;
			vtx[3].x = vtx[0].x + charInfo.width;
			vtx[3].y = vtx[0].y;
			vtx[2].x = vtx[3].x;
			vtx[2].y = vtx[1].y;
			// configure [u,v]
			vtx[0].u = (float)charInfo.x / (TexturePackerWidth - 1);
			vtx[0].v = (float)charInfo.y / (TexturePackerHeight - 1);
			vtx[1].u = vtx[0].u;
			vtx[1].v = (float)(charInfo.y + charInfo.height) / (TexturePackerHeight - 1);
			vtx[3].u = (float)(charInfo.x + charInfo.width) / (TexturePackerWidth - 1);
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
			//
			vtx += 4;
			x += charInfo.adv;// charInfo.bearingX + charInfo.width;
		}
		memcpy(&drawData->drawState.scissor, &_draw.scissorRect, sizeof(_draw.scissorRect));
		drawData->type = UIRectangle;
		//
		return drawData;
	}

	// -------------------------------------------------------------------------------------
	//	runtime drawing
	// -------------------------------------------------------------------------------------

	void UIRenderer::beginBuild(uint32_t _flightIndex)
	{
		this->m_flightIndex = _flightIndex;
		this->m_meshBufferIndex = 0;
		auto& meshBuffers = m_meshBuffers[m_flightIndex];
		for (auto& meshBuffer : meshBuffers) {
			meshBuffer.clear();
		}
	}

	void UIRenderer::buildDrawCall( const UIDrawData* _drawData )
	{
		auto& renderbles = m_renderables[m_flightIndex];
		auto& meshBuffers = m_meshBuffers[m_flightIndex];
		auto& meshBuffer = meshBuffers[m_meshBufferIndex];
		//
		if (!meshBuffer.pushVertices(_drawData)) {
			// create a new mesh buffer
			meshBuffers.resize(meshBuffers.size() + 1);
			meshBuffer = meshBuffers.back();
			auto renderable = m_material->createRenderable();
			renderbles.push_back(renderable);
			meshBuffer.initialize(m_context, renderable, MaxVertexCount);
			//
			bool rst = meshBuffer.pushVertices(_drawData);
			assert(rst);
		}
	}

	void UIRenderer::endBuild()
	{
		std::vector<UIMeshBuffer>& meshBuffers = m_meshBuffers[m_flightIndex];
		for (UIMeshBuffer& meshBuffer : meshBuffers) {
			meshBuffer.flushMeshBuffer();
		}
		// enum the draw call & call draw function
	}

	void UIRenderer::render(IRenderPass* _renderPass, float _width, float _height)
	{
		Nix::Viewport vp;
		vp.x = vp.y = 0;
		vp.zNear = 0; vp.zFar = 1.0f;
		vp.width = _width;
		vp.height = _height;
		m_pipeline->setViewport(vp);
		Nix::Scissor ss;
		ss.origin = { 0, 0 };
		ss.size = { 512, 512};
		m_pipeline->setScissor(ss);
		_renderPass->bindPipeline(m_pipeline);
		_renderPass->bindArgument(m_argument);
		//
		std::vector<UIMeshBuffer>& meshBuffers = m_meshBuffers[m_flightIndex];
		for (UIMeshBuffer& meshBuffer : meshBuffers) {
			meshBuffer.draw(_renderPass, m_argument, m_pipeline, _width, _height);
		}
	}

}