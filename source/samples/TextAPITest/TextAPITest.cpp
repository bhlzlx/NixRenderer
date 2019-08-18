#define NIX_JP_IMPLEMENTATION
#include <NixJpDecl.h>
#include <NixRenderer.h>
#include <NixUIRenderer.h>
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>
#include "TextAPITest.h"
#include <NixUIRenderer.h>
#include <random>

#undef GetObject

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


	UIRenderer::RichTextDraw richTextDrawFromFile(const char* _file, Nix::IArchieve* _archieve) {
		struct StringItem {
			uint32_t font;
			uint32_t size;
			std::string color;
			std::string text;
			NIX_JSON(font, size, color, text)
		};

		struct RichSegments {
			std::vector<StringItem> segments;
			NIX_JSON(segments)
		} segments;

		Nix::TextReader textReader;
		textReader.openFile(_archieve, std::string(_file));
		//
		segments.parse(textReader.getText());

		UIRenderer::RichTextDraw richTextDraw;

		if (segments.segments.size()) {
			for (auto& segment : segments.segments) {
				std::u16string u16 = utf8_to_utf16le(segment.text);
				UIRenderer::RichChar ch;
				uint32_t color;
				sscanf(segment.color.c_str(), "%x", &color);
				ch.color = color;
				ch.font = segment.font;
				ch.size = segment.size;
				for (size_t i = 0; i < u16.length(); ++i) {
					ch.code = u16[i];
					richTextDraw.vecChar.push_back(ch);
				}
			}
		}
		return richTextDraw;
	}

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

		m_draw = richTextDrawFromFile("text/sampleText.json", m_archieve);
		m_draw.halign = UIHoriAlign::UIAlignHoriMid;
		m_draw.valign = UIVertAlign::UIAlignTop;
		m_draw.calign = UIVertAlign::UIAlignBottom;
		m_draw.rect = { {0, 0},{512, 512} };

		m_plainDraw.colorMask = 0xff0000ff;
		m_plainDraw.fontId = 1;
		m_plainDraw.fontSize = 48;
		m_plainDraw.halign = UIHoriAlign::UIAlignLeft;
		m_plainDraw.valign = UIVertAlign::UIAlignTop;
		m_plainDraw.length = 12;
		m_plainDraw.text = u"Hello,World!";
		m_plainDraw.rect = {
			{0,0},{256, 256}
		};

		Nix::Rect<float> rc;
		m_drawData = m_uiRenderer->build(m_draw,true, nullptr, rc);
		m_plainDrawData = m_uiRenderer->build(m_plainDraw, nullptr, rc);
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
		//	m_uiRenderer->buildDrawCall(m_plainDrawData,drawState);
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
		return "GDI API Test";
	}
	inline uint32_t TextAPITest::rendererType() {
		return 0;
	}
}

Nix::TextAPITest theapp;

NixApplication* GetApplication() {
	return &theapp;
}