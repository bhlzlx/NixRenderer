#include <NixApplication.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <NixJpDecl.h>
#include <NixRenderer.h>

#include <nix/io/archieve.h>
#include <cstdio>
#include <cassert>

#ifdef _WIN32
    #include <Windows.h>
#endif

namespace nix {
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
		uint32_t					m_matLocal;
		IArgument*					m_argInstance;
		IRenderable*				m_renderable;
		IPipeline*					m_pipeline;

		ITexture*					m_texture;

		//
		virtual bool initialize(void* _wnd, nix::IArchieve* _archieve ) {
			printf("%s", "Triangle is initializing!");

			HMODULE library = ::LoadLibrary("NixVulkan.dll");
			assert(library);

			typedef IDriver*(* PFN_CREATE_DRIVER )();

			PFN_CREATE_DRIVER createDriver = reinterpret_cast<PFN_CREATE_DRIVER>(::GetProcAddress(library, "createDriver"));

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
			nix::TextReader mtlReader;
			mtlReader.openFile(_archieve, "material/triangle.json");
			mtlDesc.parse(mtlReader.getText());
			RenderPassDescription rpDesc;
			nix::TextReader rpReader;
			rpReader.openFile(_archieve, "renderpass/swapchain.json");
			rpDesc.parse(rpReader.getText());


			m_material = m_context->createMaterial(mtlDesc);

			TextureDescription texDesc;
			texDesc.depth = 1;
			texDesc.format = NixRGBA8888_UNORM;
			texDesc.height = 64;
			texDesc.width = 64;
			texDesc.mipmapLevel = 1;
			texDesc.type = nix::TextureType::Texture2D;

			m_texture = m_context->createTexture( texDesc );

			m_argCommon = m_material->createArgument(0);
			bool rst = false;
			rst = m_argCommon->getSampler("samBase", &m_samBase);
			rst = m_argCommon->getUniformBlock("GlobalArgument", &m_matGlobal);
			m_argInstance = m_material->createArgument(1);
			rst = m_argInstance->getUniformBlock("LocalArgument", &m_matLocal);

			SamplerState ss;
			m_argCommon->setSampler(m_samBase, ss, m_texture);


 			//m_context->createMaterial();


			return true;
		}

		virtual void resize(uint32_t _width, uint32_t _height) {
			printf("resized!");
			m_context->resize(_width, _height);
		}

		virtual void release() {
			printf("destroyed");
		}

		virtual void tick() {
			if (m_context->beginFrame()) {
				m_mainRenderPass->begin(m_primQueue);
				m_mainRenderPass->end();
				m_context->endFrame();
			}			
		}

		virtual const char * title() {
			return "Triangle";
		}

		virtual uint32_t rendererType() {
			return 0;
		}
	};
}

nix::Triangle theapp;

NixApplication* GetApplication() {
    return &theapp;
}