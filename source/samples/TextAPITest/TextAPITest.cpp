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
#include <random>

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
		m_draw.valign = UIVertAlign::UIAlignTop;
		m_draw.calign = UIVertAlign::UIAlignBottom;
		m_draw.rect = { {0, 0},{512, 512} };

		//

		char16_t richtext[] = u"　抬头看窗外车走走停停，人来人往，记起的，忘了的，都已成为过去。很多时候我们都希望自己可以在坚强中遗忘，于是任时间慢慢地腐蚀我们的心灵，可以看见一些枯竭了的事情在我们的脑海慢慢消失。年轻的我们很在乎，很在乎我们的每一点进步，才发现一些变化是在不经意间的。";

		m_draw.vecChar;


		static std::default_random_engine random(time(NULL));
		std::uniform_int_distribution<int> fontDis(0,3);
		std::uniform_int_distribution<int> colorDis(0x0, 0x77);
		std::uniform_int_distribution<int> sizeDis(24, 48);

		for (uint32_t i = 0; i < sizeof(richtext) / sizeof(char16_t) - 1; ++i) {
			UIRenderer::RichChar rc;
			rc.code = richtext[i];
			rc.color = colorDis(random) << 24 | colorDis(random) << 16 | colorDis(random) << 8 | 0xff;
			rc.font = 0;// fontDis(random);
			rc.size = 12;// sizeDis(random)& (~1);
			m_draw.vecChar.push_back(rc);
		}
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