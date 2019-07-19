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
	inline bool Triangle::initialize(void* _wnd, Nix::IArchieve* _archieve) {
		printf("%s", "Triangle is initializing!");

		HMODULE library = ::LoadLibraryA("NixVulkan.dll");
		assert(library);
		HMODULE packerLibrary = ::LoadLibraryA("TexturePacker.dll");
		assert(packerLibrary);

		typedef IDriver* (*PFN_CREATE_DRIVER)();
		PFN_CREATE_DRIVER createDriver = reinterpret_cast<PFN_CREATE_DRIVER>(::GetProcAddress(library, "createDriver"));
		PFN_CREATE_TEXTURE_PACKER createTexturePacker = reinterpret_cast<PFN_CREATE_TEXTURE_PACKER>(::GetProcAddress(packerLibrary, "CreateTexturePacker"));

		// \ 1. create driver
		m_driver = createDriver();
		m_driver->initialize(_archieve, DeviceType::DiscreteGPU);
		// \ 2. create context
		m_context = m_driver->createContext(_wnd);

		m_mainRenderPass = m_context->getRenderPass();
		m_primQueue = m_context->getGraphicsQueue(0);

		RpClear clear;
		clear.colors[0] = { 1.0, 1.0, 0.0f, 1.0f };
		clear.depth = 1.0f;
		clear.stencil = 1;
		m_mainRenderPass->setClear(clear);
		//
		MaterialDescription mtlDesc;
		Nix::TextReader mtlReader;
		mtlReader.openFile(_archieve, "material/triangle.json");
		mtlDesc.parse(mtlReader.getText());
		RenderPassDescription rpDesc;
		Nix::TextReader rpReader;
		rpReader.openFile(_archieve, "renderpass/swapchain.json");
		rpDesc.parse(rpReader.getText());
		rpDesc.colors[0].format = m_context->swapchainColorFormat();
		rpDesc.depthStencil.format = m_driver->selectDepthFormat(true);

		TextureDescription texDesc;
		texDesc.depth = 1;
		texDesc.format = NixR8_UNORM;
		texDesc.height = 256;
		texDesc.width = 256;
		texDesc.mipmapLevel = 1;
		texDesc.type = Nix::TextureType::Texture2D;
		//
		m_texture = m_context->createTexture(texDesc);
		m_texturePacker = createTexturePacker(m_texture, 0);
		m_font = new Font();
		m_font->initialize(_archieve, "font/hwzhsong.ttf", 0, m_texturePacker);
		
		const char16_t message[] = u"你好世界中国字符，】【；‘，。/《》？，~`!@#$%^&*()_+=-0987654321[];',/{}:\"<>?\\|！中国字符、】【；‘，。/《》？一";
		for (auto c : message) {
			FontCharactor fc;
			fc.charCode = c;
			fc.size = 28;
			m_font->getCharacter(fc);
		}


		//uint32_t data[64 * 64];
		//
		//for (uint32_t i = 0; i < 10000; ++i) {
		//	static std::default_random_engine random(time(NULL));
		//	std::uniform_int_distribution<int> dis1(8, 24);
		//	int randW = dis1(random);
		//	int randH = dis1(random);
		//	Rect<uint16_t> rc;
		//	std::uniform_int_distribution<int> dis2(31, 255);
		//	uint32_t r = dis2(random);
		//	uint32_t g = dis2(random);
		//	uint32_t b = dis2(random);
		//	for (uint32_t j = 0; j < randW * randH; ++j) {
		//		data[j] = 0xff000000 | g << 16 | b << 8 | r;
		//	}
		//	auto ret = m_texturePacker->insert((uint8_t*)data, randW * randH * 4, randW, randH, rc);
		//	if (!ret) {
		//		break;
		//	}
		//}

		bool rst = false;
		m_material = m_context->createMaterial(mtlDesc); {
			{ // graphics pipeline 
				m_pipeline = m_material->createPipeline(rpDesc);
			}
			{ // arguments
				m_argCommon = m_material->createArgument(0);
				rst = m_argCommon->getSampler("samBase", &m_samBase);
				rst = m_argCommon->getUniformBlock("GlobalArgument", &m_matGlobal);
				m_argInstance = m_material->createArgument(1);
				rst = m_argInstance->getUniformBlock("LocalArgument", &m_matLocal);
				SamplerState ss;
				m_argCommon->setSampler(m_samBase, ss, m_texture);
			}
			{ // renderable
				m_renderable = m_material->createRenderable();
				m_vertexBuffer = m_context->createStaticVertexBuffer(PlaneVertices, sizeof(PlaneVertices));
				m_indexBuffer = m_context->createIndexBuffer(PlaneIndices, sizeof(PlaneIndices));
				m_renderable->setVertexBuffer(m_vertexBuffer, 0, 0);
				m_renderable->setIndexBuffer(m_indexBuffer, 0);
			}
		}
		return true;
	}
	inline void Triangle::resize(uint32_t _width, uint32_t _height) {
		printf("resized!");
		m_context->resize(_width, _height);
		Viewport vp = {
			0.0f, 0.0f, (float)_width, (float)_height, 0.0f, 1.0f
		};
		Scissor ss;
		ss.origin = { 0, 0 };
		ss.size = { (int)_width, (int)_height };
		//
		m_pipeline->setViewport(vp);
		m_pipeline->setScissor(ss);
	}
	inline void Triangle::release() {
		printf("destroyed");
		m_context->release();
	}
	inline void Triangle::tick() {

		static uint64_t tickCounter = 0;

		tickCounter++;

		float imageIndex = (tickCounter / 1024) % 4;

		if (m_context->beginFrame()) {
			m_mainRenderPass->begin(m_primQueue); {
				glm::mat4x4 identity;
				m_argCommon->setUniform(m_matGlobal, 0, &identity, 64);
				m_argCommon->setUniform(m_matGlobal, 64, &identity, 64);
				m_argCommon->setUniform(m_matGlobal, 128, &imageIndex, 4);
				m_argInstance->setUniform(m_matLocal, 0, &identity, 64);
				//
				m_mainRenderPass->bindPipeline(m_pipeline);
				m_mainRenderPass->bindArgument(m_argCommon);
				m_mainRenderPass->bindArgument(m_argInstance);
				//
				m_mainRenderPass->drawElements(m_renderable, 0, 6);
			}
			m_mainRenderPass->end();

			m_context->endFrame();
		}
	}
	inline const char* Triangle::title() {
		return "Text Rendering( texture packer test )";
	}
	inline uint32_t Triangle::rendererType() {
		return 0;
	}
}

Nix::Triangle theapp;

NixApplication* GetApplication() {
	return &theapp;
}