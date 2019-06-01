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
		IArgument*					m_argInstance;
		IRenderable*				m_renderable;
		IPipeline*					m_pipeline;
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
			mtlReader.openFile(_archieve, "materials/triangle.json");
			mtlDesc.parse(mtlReader.getText());


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