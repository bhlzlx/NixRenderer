#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#define NIX_JP_IMPLEMENTATION
#include <NixJpDecl.h>
#include <NixRenderer.h>
#include <NixUIRenderer.h>
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>
#include "TextAPITest.h"
#include <NixUIRenderer.h>

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
	bool TextAPITest::initialize(void* _wnd, Nix::IArchieve* _archieve) {
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
		m_archieve = _archieve;

		m_mainRenderPass = m_context->getRenderPass();
		m_primQueue = m_context->getGraphicsQueue(0);

		RpClear clear;
		clear.colors[0] = { 1.0f, 1.0f, 1.0f, 1.0f };
		clear.depth = 1.0f;
		clear.stencil = 1;
		m_mainRenderPass->setClear(clear);
		
		m_uiRenderer = new UIRenderer();
		UIRenderConfig uiconfig;
		TextReader uiConfigReader;
		uiConfigReader.openFile(m_archieve, "ui/config.json");
		uiconfig.parse(uiConfigReader.getText());
		m_uiRenderer->initialize(m_context, m_archieve, uiconfig);

		m_draw.halign = UIHoriAlign::UIAlignHoriMid;
		m_draw.valign = UIVertAlign::UIAlignVertMid;
		m_draw.rect = { {0, 0},{512, 512} };
		m_draw.vecChar = {
			{ 0, u'±‡', 32, 0x880000ff },
			{ 1, u'“Î', 32, 0x888800ff },
			{ 2, u'‘≠', 48, 0x008800ff },
			{ 1, u'¿Ì', 24, 0x800664ff },
			{ 2, u'p', 32, 0xf00f00ff },
			{ 1, u'f', 36, 0xff8800ff },
		};
		Nix::Rect<float> rc;
		m_drawData = m_uiRenderer->build(m_draw,true, nullptr, rc);
		//
		return true;
	}

	inline void TextAPITest::resize(uint32_t _width, uint32_t _height) {
		//
		m_context->resize(_width, _height);
		Viewport vp = {
			0.0f, 0.0f, (float)_width, (float)_height, 0.0f, 1.0f
		};
		m_scissor.origin = { 0, 0 };
		m_scissor.size = { (int)_width, (int)_height };
		//
		m_width = (float)_width;
		m_height = (float)_height;

	}
	inline void TextAPITest::release() {
		printf("destroyed");
		m_context->release();
	}
	inline void TextAPITest::tick() {

		static uint64_t tickCounter = 0;
		tickCounter++;
		tickCounter %= 3;

		if (m_context->beginFrame()) {
			UIDrawState drawState;
			drawState.setScissor(m_scissor);
			m_uiRenderer->beginBuild(tickCounter);
			m_uiRenderer->buildDrawCall(m_drawData,drawState);
			m_uiRenderer->endBuild();

			m_mainRenderPass->begin(m_primQueue); {
				//
				m_uiRenderer->render(m_mainRenderPass, m_width, m_height);
			}
			m_mainRenderPass->end();
			m_context->endFrame();
		}
	}
	inline const char* TextAPITest::title() {
		return "Text Rendering( texture packer test )";
	}
	inline uint32_t TextAPITest::rendererType() {
		return 0;
	}
}

Nix::TextAPITest theapp;

NixApplication* GetApplication() {
	return &theapp;
}