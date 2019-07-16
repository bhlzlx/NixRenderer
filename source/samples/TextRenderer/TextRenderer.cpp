#include <NixApplication.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <NixJpDecl.h>
#include <NixRenderer.h>
#include <TexturePacker/TexturePacker.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

#include <nix/io/archieve.h>
#include <cstdio>
#include <cassert>
#include <random>
#include <ctime>

#ifdef _WIN32
    #include <Windows.h>
#endif

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

	class Triangle : public NixApplication {
	private:
		IDriver*					m_driver;
		IContext*					m_context;
		IRenderPass*				m_mainRenderPass;
		IGraphicsQueue*				m_primQueue;
		//
		IMaterial*					m_material;

		IArgument*					m_argCommon;
		uint32_t					m_samBase;
		uint32_t					m_matGlobal;

		IArgument*					m_argInstance;
		uint32_t					m_matLocal;

		IRenderable*				m_renderable;

		IBuffer*					m_vertexBuffer;
		IBuffer*					m_indexBuffer;

		IPipeline*					m_pipeline;

		ITexture*					m_texture;
		ITexturePacker*				m_texturePacker;

		//
		virtual bool initialize(void* _wnd, Nix::IArchieve* _archieve ) {
			printf("%s", "Triangle is initializing!");

			HMODULE library = ::LoadLibraryA("NixVulkan.dll");
			assert(library);
			HMODULE packerLibrary = ::LoadLibraryA("TexturePacker.dll");
			assert(packerLibrary);

			typedef IDriver*(* PFN_CREATE_DRIVER )();
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
			texDesc.format = NixRGBA8888_UNORM;
			texDesc.height = 256;
			texDesc.width = 256;
			texDesc.mipmapLevel = 1;
			texDesc.type = Nix::TextureType::Texture2D;
			//
			m_texture = m_context->createTexture(texDesc);
			m_texturePacker = createTexturePacker(m_texture, 0);

			uint32_t data[16*16];

			for (uint32_t i = 0; i < 25; ++i) {
				static std::default_random_engine random(time(NULL));
				std::uniform_int_distribution<int> dis1(4, 16);
				int randW = dis1(random);
				int randH = dis1(random);
				Rect<uint16_t> rc;
				std::uniform_int_distribution<int> dis2(31, 255);
				uint32_t r = dis2(random);
				uint32_t g = dis2(random);
				uint32_t b = dis2(random);
				for (uint32_t j = 0; j < randW * randH; ++j) {
					data[j] = 0xff000000 | g << 16 | b << 8 | r ;
				}
				m_texturePacker->insert(std::to_string(i).c_str(), (uint8_t*)data, randW * randH * 4, randW, randH, rc);
			}

			//stbtt_fontinfo font;
			//int i, j, ascent, baseline, ch = 0;
			//float scale, xpos = 2; // leave a little padding in case the character extends left
			//char* text = "Heljo World!"; // intentionally misspelled to show 'lj' brokenness
			//IFile* fontFile = _archieve->open("font/font00.ttf"); assert(fontFile);
			//IFile* fontBuffer = CreateMemoryBuffer(fontFile->size());
			//fontFile->read(fontFile->size(), fontBuffer);
			//fontFile->release();
			//int fontCode = stbtt_InitFont(&font, (const unsigned char*)fontBuffer->constData(), 0);
			//scale = stbtt_ScaleForPixelHeight(&font, 15);
			//stbtt_GetFontVMetrics(&font, &ascent, 0, 0);
			//baseline = (int)(ascent * scale);
			//
			//while (text[ch]) {
			//	int advance, lsb, x0, y0, x1, y1;
			//	float x_shift = xpos - (float)floor(xpos);
			//	stbtt_GetCodepointHMetrics(&font, text[ch], &advance, &lsb);
			//	stbtt_GetCodepointBitmapBoxSubpixel(&font, text[ch], scale, scale, x_shift, 0, &x0, &y0, &x1, &y1);
			//	stbtt_MakeCodepointBitmapSubpixel(&font, &screen[baseline + y0][(int)xpos + x0], x1 - x0, y1 - y0, 79, scale, scale, x_shift, 0, text[ch]);
			//	// note that this stomps the old data, so where character boxes overlap (e.g. 'lj') it's wrong
			//	// because this API is really for baking character bitmaps into textures. if you want to render
			//	// a sequence of characters, you really need to render each bitmap to a temp buffer, then
			//	// "alpha blend" that into the working buffer
			//	xpos += (advance * scale);
			//	if (text[ch + 1])
			//		xpos += scale * stbtt_GetCodepointKernAdvance(&font, text[ch], text[ch + 1]);
			//	++ch;
			//}

			//for (j = 0; j < 20; ++j) {
			//	for (i = 0; i < 78; ++i)
			//		putchar(" .:ioVM@"[screen[j][i] >> 5]);
			//	putchar('\n');
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

		virtual void resize(uint32_t _width, uint32_t _height) {
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

		virtual void release() {
			printf("destroyed");
			m_context->release();
		}

		virtual void tick() {

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

		virtual const char * title() {
			return "Triangle (with Texture2D Array)";
		}

		virtual uint32_t rendererType() {
			return 0;
		}
	};
}

Nix::Triangle theapp;

NixApplication* GetApplication() {
    return &theapp;
}