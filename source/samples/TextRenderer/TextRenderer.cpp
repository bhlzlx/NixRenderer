#define NIX_JP_IMPLEMENTATION
#include <NixJpDecl.h>
#include <NixRenderer.h>
#include <NixUIRenderer.h>
#include <NixUIImage.h>
#include <NixUILabel.h>
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
		m_archieve = _archieve;

		m_mainRenderPass = m_context->getRenderPass();
		m_primQueue = m_context->getGraphicsQueue(0);

		RpClear clear;
		clear.colors[0] = { 0.5f, 0.4f, 0.4f, 1.0f };
		clear.depth = 1.0f;
		clear.stencil = 1;
		m_mainRenderPass->setClear(clear);

		UISystem::StandardScreenWidth = UISystem::StandardScreenHeight = 512;
		m_uiSystem = UISystem::getInstance();
		m_uiSystem->initialize( m_context, m_archieve );
		m_uiRenderer = m_uiSystem->getRenderer();

		{
			UIImage* img = new UIImage();
			img->setImage("button_00.png");
			img->setRect({ 0, 0, 32, 32 });
			img->setAlign({ UIAlignBottom, UIAlignLeft });
			UIWidget* root = m_uiSystem->getRootWidget();
			root->addSubWidget(img);
		}

		{
			UIImage* img = new UIImage();
			img->setImage("notfound.png");
			img->setRect({ 16, 16, 64, 64 });
			img->setAlign({ UIAlignBottom, UIAlignLeft });
			UIWidget* root = m_uiSystem->getRootWidget();
			root->addSubWidget(img);
		}

		{
			UIImage* img = new UIImage();
			img->setImage("button_00.png");
			img->setRect({ 0, 48, 64, 64 });
			img->setAlign({ UIAlignBottom, UIAlignLeft });
			img->setColor( 0xff000088 );

			UILabel* lbl = new UILabel();
			lbl->setRect({ 0, 0, 64, 64 });
			lbl->setColor(0xffff00ff);
			lbl->setFont(1);
			lbl->setText({u'一',u'个',u'按',u'钮' });
			img->addSubWidget(lbl);

			UIWidget* root = m_uiSystem->getRootWidget();
			root->addSubWidget(img);
		}
		
		//
		return true;
	}

	inline void TextSample::resize(uint32_t _width, uint32_t _height) {
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
		//
		m_uiSystem->onResize(_width, _height);
	}
	inline void TextSample::release() {
		printf("destroyed");
		m_context->release();
	}
	inline void TextSample::tick() {

		static uint64_t tickCounter = 0;
		tickCounter++;

		m_uiSystem->onTick();

		if (m_context->beginFrame()) {
			m_mainRenderPass->begin(m_primQueue); {
				m_uiRenderer->render(m_mainRenderPass, m_width, m_height);
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