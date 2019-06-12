#include <NixApplication.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <NixJpDecl.h>
#include <NixRenderer.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <nix/io/archieve.h>
#include <cstdio>
#include <cassert>
#include <string>

// android ndk headers
#include <jni.h>
#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <dlfcn.h>

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

	class NixApp : public NixApplication {
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

		//
		virtual bool initialize(void* _wnd, Nix::IArchieve* _archieve ) {
			printf("%s", "NixApp is initializing!");
            void* library = dlopen("libNixVulkan.so",RTLD_NOW | RTLD_LOCAL);
			assert(library);

			typedef IDriver*(* PFN_CREATE_DRIVER )();

            PFN_CREATE_DRIVER createDriver = reinterpret_cast<PFN_CREATE_DRIVER>(dlsym( library, "createDriver" ));

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

			TextureDescription texDesc;
			texDesc.depth = 1;
			texDesc.format = NixRGBA8888_UNORM;
			texDesc.height = 64;
			texDesc.width = 64;
			texDesc.mipmapLevel = 1;
			texDesc.type = Nix::TextureType::Texture2D;

			Nix::IFile * texFile = _archieve->open("texture/texture_etc2.ktx");
			//Nix::IFile * texFile = _archieve->open("texture/texture_bc3.dds");
			Nix::IFile* texMem = CreateMemoryBuffer(texFile->size());
			texFile->read(texFile->size(), texMem);
			//m_texture = m_context->createTexture(texDesc);
			m_texture = m_context->createTextureKTX(texMem->constData(), texMem->size());

            texDesc.format = NixDepth32F;
			auto sampleD32Tex = m_context->createTexture( texDesc, Nix::TextureUsageDepthStencilAttachment );

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
			if (m_context->beginFrame()) {
				m_mainRenderPass->begin(m_primQueue); {
					glm::mat4x4 identity;
					m_argCommon->setUniform(m_matGlobal, 0, &identity, 64);
					m_argCommon->setUniform(m_matGlobal, 64, &identity, 64);
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
			return "NixApp";
		}

		virtual uint32_t rendererType() {
			return 0;
		}
	};
}

Nix::NixApp theapp;

NixApplication* GetApplication() {
    return &theapp;
}


/*
    public native void NixVkInit(Object surface, int width, int height, Object assetManager);
    public native void NixVkDraw();
    public native void NixVkDestroy();
*/

extern "C" JNIEXPORT void JNICALL
Java_com_kusugawa_NixRenderer_VkRenderer_onInitialize(
                                            JNIEnv*     _env,
                                            jobject     _this,
                                            jobject     _surface,
                                            jint        _width,
                                            jint        _height,
                                            jstring		_documentPath,
                                            jobject     _assetManager
                                            )
{
	NixApplication* application = GetApplication();
	ANativeWindow* nativeWindow = ANativeWindow_fromSurface( _env, _surface );
    int length = (_env)->GetStringUTFLength(_documentPath);
    const char* jstr = (_env)->GetStringUTFChars(_documentPath, nullptr);
    std::string documentPath( jstr, jstr+length );
    int size = 0;
    Nix::IArchieve* arch = Nix::CreateStdArchieve(documentPath);
	application->initialize(nativeWindow, arch);
	//application->resize( _width, _height );
    return;
}

extern "C" JNIEXPORT void JNICALL
Java_com_kusugawa_NixRenderer_VkRenderer_onDraw(
                                            JNIEnv*     _env,
                                            jobject     _this
                                            )
{
    NixApplication* application = GetApplication();
    application->tick();
    return;
}

extern "C" JNIEXPORT void JNICALL
Java_com_kusugawa_NixRenderer_VkRenderer_onSizeChanged(
                                            JNIEnv*     _env,
                                            jobject     _this,
                                            jint        _width,
                                            jint        _height
                                            )
{
    NixApplication* application = GetApplication();
    application->resize( _width, _height );
}

extern "C" JNIEXPORT void JNICALL
Java_com_kusugawa_NixRenderer_VkRenderer_onDestroy(
                                            JNIEnv*     _env,
                                            jobject     _this
                                            )
{
    NixApplication* application = GetApplication();
    application->release();
    return;
}