#include <NixJpDecl.h>
#include <NixRenderer.h>
#include "NixUIRenderer.h"
#include <nix/io/archieve.h>
#include "TexturePacker/TexturePacker.h"

namespace Nix {

	bool UIRenderer::initialize(IContext* _context, IArchieve* _archieve, const UIRenderConfig& _config) {
		m_context = _context;
		m_archieve = _archieve;
		MaterialDescription mtl;
		strcpy(mtl.shaders[ShaderModuleType::VertexShader].name, "ui/ui.vert");
		mtl.shaders[ShaderModuleType::VertexShader].type = ShaderModuleType::VertexShader;
		strcpy(mtl.shaders[ShaderModuleType::FragmentShader].name, "ui/ui.frag");
		mtl.shaders[ShaderModuleType::FragmentShader].type = ShaderModuleType::FragmentShader;
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
			mtl.renderState.writeMask = 0x0f;
		}
		mtl.topologyMode = Nix::TMTriangleList;
		//
		mtl.vertexLayout.attributeCount = 2;
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
		mtl.vertexLayout.attributes[1].type = Nix::VertexTypeFloat2;
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
		// read texturepacker.json
		/* like this
		{
			"textures" : [{
				"file" : "texture/gui/packer0.png",
				"table"  : "texture/gui/packer0.json"
			}]
		}
		*/
		// read packed textures
		{
			bool rst = m_textureManager.initialize(_context, _archieve, 2, (uint32_t)_config.textures.size(), 1);
			if (rst == false) {
				return false;
			}
		}
		for (auto& item : _config.textures) {
			m_textureManager.addPackedImage(item.table.c_str(), item.file.c_str());
		}
		// create renderable objects / one renderable per frame
		for (uint32_t i = 0; i < MaxFlightCount; ++i) {
			m_renderables[i].push_back(m_material->createRenderable());
			m_meshBuffers[i].resize(1);
			m_meshBuffers[i].back().initialize(_context, m_renderables[i][0], MaxVertexCount);
		}
		// add fonts
		for (auto& font : _config.fonts) {
			uint32_t code = m_textureManager.getFontTextureManger()->addFont(font.c_str());
			if (-1 == code) {
				break;
			}
		}
		m_textureArray = m_textureManager.getTexture();
		m_uniformBuffer = m_context->createUniformBuffer(8 * 1024 * 16);// 最多同时上屏约 16k 个四边形
		m_argument = m_material->createArgument(0);
		if (!m_argument || !m_textureArray) {
			return false;
		}
		SamplerState ss;
		uint32_t samplerLoc;
		uint32_t textureLoc;
		uint32_t uniformLoc;
		m_argument->getSamplerLocation("uiSampler", samplerLoc);
		m_argument->getTextureLocation("uiTextureArray", textureLoc);
		m_argument->getUniformBlockLocation("RectParams", uniformLoc);
		m_argument->bindSampler(samplerLoc, ss);
		m_argument->bindTexture(textureLoc, m_textureArray);
		m_argument->bindUniformBuffer(uniformLoc, m_uniformBuffer);
		return true;
	}

	void UIRenderer::setScreenSize(int _width, int _height)
	{
		m_width = _width;
		m_height = _height;
	}

	uint32_t UIRenderer::addFont(const char* _filepath)
	{
		return m_textureManager.getFontTextureManger()->addFont(_filepath);
	}

	// -------------------------------------------------------------------------------------
	//	runtime drawing
	// -------------------------------------------------------------------------------------

	void UIRenderer::destroyDrawData(UIDrawData* _draw)
	{
		m_vertexMemoryHeap.free(_draw->vertexBufferAllocation);
		m_drawDataPool.deleteElement(_draw);
	}

	void UIRenderer::beginBuild(uint32_t _flightIndex)
	{
		this->m_flightIndex = _flightIndex;
		this->m_meshBufferIndex = 0;
		auto& meshBuffers = m_meshBuffers[m_flightIndex];
		for (auto& meshBuffer : meshBuffers) {
			meshBuffer.clear();
		}
		m_drawState.scissor.origin = { 0 , 0 };
		m_drawState.scissor.size = { m_width, m_height };
	}

	void UIRenderer::buildDrawCall(const UIDrawData* _drawData, const UIDrawState& _state)
	{
		auto& renderbles = m_renderables[m_flightIndex];
		auto& meshBuffers = m_meshBuffers[m_flightIndex];
		auto& meshBuffer = meshBuffers[m_meshBufferIndex];
		//
		if (!meshBuffer.pushVertices(_drawData, _state, m_drawState)) {
			// create a new mesh buffer
			meshBuffers.resize(meshBuffers.size() + 1);
			meshBuffer = meshBuffers.back();
			auto renderable = m_material->createRenderable();
			renderbles.push_back(renderable);
			meshBuffer.initialize(m_context, renderable, MaxVertexCount);
			//
			bool rst = meshBuffer.pushVertices(_drawData, _state, m_drawState);
			assert(rst);
		}
		// update current scissor state
		m_drawState = _state;
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
		Nix::Scissor ss;
		ss.origin = { 0, 0 };
		ss.size = { (int)_width, (int)_height };
		//
		_renderPass->setViewport(vp);
		_renderPass->setScissor(ss);
		_renderPass->bindPipeline(m_pipeline);
		_renderPass->bindArgument(m_argument);
		//
		uint32_t totalRectCount = 0;
		std::vector<UIMeshBuffer>& meshBuffers = m_meshBuffers[m_flightIndex];
		for (UIMeshBuffer& meshBuffer : meshBuffers) {
			struct Constants {
				float screenWidth;
				float screenHeight;
				uint32_t baseRectIndex;
			} constants;
			constants.screenWidth = _width - 1;
			constants.screenHeight = _height - 1;
			constants.baseRectIndex = totalRectCount;
			_renderPass->bindArgument(m_argument);
			m_argument->updateUniformBuffer(m_uniformBuffer, meshBuffer.getUniformData(), totalRectCount * sizeof(UIUniformElement), meshBuffer.getUniformLength());
			meshBuffer.draw(_renderPass, m_argument, totalRectCount);
		}
	}

}
