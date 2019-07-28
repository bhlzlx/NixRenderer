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
		clear.colors[0] = { .5f, .5f, .5f, 1.0f };
		clear.depth = 1.0f;
		clear.stencil = 1;
		m_mainRenderPass->setClear(clear);

		m_uiRenderer.initialize(m_context, _archieve);
		m_uiRenderer.addFont("font/hwzhsong.ttf");

		Nix::UIRenderer::TextDraw draw;
		draw.fontId = 0;
		draw.fontSize = 24;
		draw.alpha = 1.0f;
		draw.length = 13;
		draw.text = u"ÄãºÃ£¬ÊÀ½ç£¡phantom";
		draw.original = { 32, 32 };
		draw.scissor.origin = {0 , 0};
		draw.scissor.size = {512, 512};

		m_drawData = m_uiRenderer.build(draw, nullptr);

		return true;
	}
	inline void TextSample::resize(uint32_t _width, uint32_t _height) {
		printf("resized!");
		m_context->resize(_width, _height);
		Viewport vp = {
			0.0f, 0.0f, (float)_width, (float)_height, 0.0f, 1.0f
		};
		Scissor ss;
		ss.origin = { 0, 0 };
		ss.size = { (int)_width, (int)_height };
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

			m_uiRenderer.beginBuild(tickCounter%MaxFlightCount);
			m_uiRenderer.buildDrawCall(m_drawData);
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