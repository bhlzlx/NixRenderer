#include "TextRenderer.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

namespace Nix {

	float PlaneVertices[] = {
	-0.5f, -0.5f, 0.0f, 0.0f, 1.0f,
	0.5f, -0.5f, 0.0f, 1.0f, 1.0f,
	0.5f, 0.5f, 0.0f, 1.0f, 0.0f,
	-0.5f, 0.5f, 0.0f, 0.0f, 0.0f,
	};

	uint16_t PlaneIndices[] = {
		0,1,2,0,2,3
	};

	//
	inline bool TextSample::initialize(void* _wnd, Nix::IArchieve* _archieve) {
		printf("%s", "Text Renderer is initializing!");

		HMODULE library = ::LoadLibraryA("NixVulkan.dll");
		assert(library);
		typedef IDriver* (*PFN_CREATE_DRIVER)();
		PFN_CREATE_DRIVER createDriver = reinterpret_cast<PFN_CREATE_DRIVER>(::GetProcAddress(library, "createDriver"));
		// \ 1. create driver
		m_driver = createDriver();
		m_driver->initialize(_archieve, DeviceType::DiscreteGPU);
		// \ 2. create context
		m_context = m_driver->createContext(_wnd);

		m_mainRenderPass = m_context->getRenderPass();
		m_primQueue = m_context->getGraphicsQueue(0);

		RpClear clear;
		clear.colors[0] = { 1.0f, 1.0f, 1.0f, 1.0f };
		clear.depth = 1.0f;
		clear.stencil = 1;
		m_mainRenderPass->setClear(clear);

		m_uiRenderer.initialize(m_context, _archieve);
		m_uiRenderer.addFont("font/hwzhsong.ttf");
		m_uiRenderer.addFont("font/font00.ttf");

		char16_t text[] = u"ÎÄ±¾¶ÔÆë²âÊÔ - powered by Vulkan!";
		char16_t text2[] = u"²Ã¼ô²âÊÔ";

		Nix::UIRenderer::TextDraw draw;
		draw.fontId = 0;
		draw.fontSize = 32;
		// RGBA
		draw.colorMask = 0x770044ff;
		draw.length = sizeof(text) / 2 - 1;
		draw.text = &text[0];
		draw.rect.origin = { 0, 0 };
		draw.rect.size = {512, 256};
		draw.halign = UIAlignHoriMid;
		draw.valign = UIAlignVertMid;

		m_drawData1 = m_uiRenderer.build(draw, nullptr);

		draw.colorMask = 0x007755ff;
		draw.length = sizeof(text2) / 2 - 1;
		draw.text = &text2[0];
		draw.fontSize = 24;
		draw.fontId = 1;
		draw.halign = UIAlignLeft;
		draw.valign = UIAlignBottom;

		m_drawData2 = m_uiRenderer.build(draw, nullptr);
		//m_uiRenderer.transformDrawData(m_drawData1, 0, -32, m_drawData2);
		m_drawData3 = m_uiRenderer.copyDrawData(m_drawData2);

		Nix::Scissor customScissor;
		customScissor.origin = { 4,4 };
		customScissor.size = { 96, 18 };
		m_uiRenderer.scissorDrawData(m_drawData2, customScissor, m_drawData3);

		Nix::UIRenderer::ImageDraw imgDraw;
		imgDraw.color = 0x878744ff;
		imgDraw.layer = 0;
		imgDraw.rect = {
			{0, 0},
			{ 512, 512 }
		};
		imgDraw.uv[0] = { 0, 0 };
		imgDraw.uv[1] = { 0, 1.0f };
		imgDraw.uv[2] = { 1.0f, 1.0f };
		imgDraw.uv[3] = { 1.0f, 0.0f };

		m_drawData4 = m_uiRenderer.build(&imgDraw, 1, nullptr);
		//
		//draw.fontId = 0;
		//draw.fontSize = 32;
		//draw.colorMask = 0xff0000ff;
		//draw.rect.origin = { 32, 96 };
		//m_drawData3 = m_uiRenderer.build(draw, nullptr);
		return true;
	}

	inline void TextSample::resize(uint32_t _width, uint32_t _height) {
		printf("resized!");
		m_context->resize(_width, _height);
		Viewport vp = {
			0.0f, 0.0f, (float)_width, (float)_height, 0.0f, 1.0f
		};
		m_scissor.origin = { 0, 0 };
		m_scissor.size = { (int)_width, (int)_height };
		//
		m_width = _width;
		m_height = _height;
	}
	inline void TextSample::release() {
		printf("destroyed");
		m_context->release();
	}
	inline void TextSample::tick() {

		static uint64_t tickCounter = 0;
		tickCounter++;

		if (m_context->beginFrame()) {

			UIDrawState state;
			state.scissor = m_scissor;
			m_uiRenderer.beginBuild(tickCounter%MaxFlightCount);
			m_uiRenderer.buildDrawCall(m_drawData1, state );
			m_uiRenderer.buildDrawCall(m_drawData3, state );
			m_uiRenderer.buildDrawCall(m_drawData4, state);
			//state.scissor.size = { 512, 512 };
			//m_uiRenderer.buildDrawCall(m_drawData3, state);
			m_uiRenderer.endBuild();

			m_mainRenderPass->begin(m_primQueue); {
				m_uiRenderer.render(m_mainRenderPass, m_width, m_height);
			}
			m_mainRenderPass->end();

			m_context->endFrame();
		}
	}
	inline const char* TextSample::title() {
		return "Text Rendering( texture packer test )";
	}
	inline uint32_t TextSample::rendererType() {
		return 0;
	}
}

Nix::TextSample theapp;

NixApplication* GetApplication() {
	return &theapp;
}