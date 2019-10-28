#define NIX_JP_IMPLEMENTATION
#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <NixJpDecl.h>
#include <NixRenderer.h>

#include <NixApplication.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <nix/io/archieve.h>
#include <cstdio>
#include <cassert>

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

		IArgument*					m_argument;
		uint32_t					m_triSamplerLoc;
		uint32_t					m_triTextureLoc;
		uint32_t					m_triTransformLoc;
		SamplerState				m_triSampler;
		ITexture*					m_triTexture;
		IBuffer*					m_triTransformMatrix;

		/*IArgument*					m_argInstance;
		uint32_t					m_matLocal;*/

		IRenderable*				m_renderable;

		IBuffer*					m_vertexBuffer;
		IBuffer*					m_indexBuffer;

		IPipeline* m_pipeline;

		IPipeline* m_computePipeline;
		IMaterial* m_computeMaterial;
		IArgument* m_computeArgument;
		ITexture* m_computeOutput;

		//
		virtual bool initialize(void* _wnd, Nix::IArchieve* _archieve) {
			printf("%s", "Triangle is initializing!");

			HMODULE library = ::LoadLibraryA("NixVulkan.dll");
			assert(library);

			typedef IDriver*(*PFN_CREATE_DRIVER)();

			PFN_CREATE_DRIVER createDriver = reinterpret_cast<PFN_CREATE_DRIVER>(::GetProcAddress(library, "createDriver"));

			// \ 1. create driver
			m_driver = createDriver();
			m_driver->initialize(_archieve, DeviceType::DiscreteGPU);
			// \ 2. create context
			m_context = m_driver->createContext(_wnd);

			MaterialDescription computeMtlDesc;
			Nix::TextReader computeMtlReader;
			computeMtlReader.openFile(_archieve, "material/triangle_compute.json");
			computeMtlDesc.parse(computeMtlReader.getText());

			IMaterial* computeMtl = m_context->createMaterial(computeMtlDesc);
			m_computePipeline = computeMtl->createComputePipeline();
			m_computeArgument = computeMtl->createArgument(0);

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
			texDesc.height = 64;
			texDesc.width = 64;
			texDesc.mipmapLevel = 1;
			texDesc.type = Nix::TextureType::Texture2D;

			//Nix::IFile * texFile = _archieve->open("texture/texture_bc3.ktx");
			//Nix::IFile * texFile = _archieve->open("texture/texture_array_bc3.ktx");
			//Nix::IFile* texMem = CreateMemoryBuffer(texFile->size());
			//texFile->read(texFile->size(), texMem);
			//m_texture = m_context->createTextureKTX(texMem->constData(), texMem->size());

			m_triTexture = m_context->createTexture(texDesc);
			m_computeOutput = m_context->createTexture(texDesc);
			m_triTransformMatrix = m_context->createUniformBuffer(64);

			Nix::IFile* fishPNG = _archieve->open("texture/fish.png");
			Nix::IFile* memPNG = CreateMemoryBuffer(fishPNG->size());
			fishPNG->read(fishPNG->size(), memPNG);

			int x, y, channel = 4;
			auto rawData = stbi_load_from_memory((const stbi_uc*)memPNG->constData(), memPNG->size(), &x, &y, &channel, 4);
			BufferImageUpload upload;
			upload.baseMipRegion = {
				0, 0, { 0 , 0 , 0 }, { (uint32_t)x, (uint32_t)y, 1}
			};
			upload.data = rawData;
			upload.length = x * y * 4;
			upload.mipCount = 1;
			upload.mipDataOffsets[0] = 0;
			m_triTexture->uploadSubData(upload);


			uint32_t inputLoc, outputLoc;
			m_computeArgument->getStorageImageLocation("imageIn", inputLoc);
			m_computeArgument->getStorageImageLocation("imageOut", outputLoc);

			m_computeArgument->bindStorageImage(inputLoc, m_triTexture);
			m_computeArgument->bindStorageImage(outputLoc, m_computeOutput);

			bool rst = false;
			m_material = m_context->createMaterial(mtlDesc); {
				{ // graphics pipeline 
					m_pipeline = m_material->createPipeline(rpDesc);
				}
				{ // arguments
					m_argument = m_material->createArgument(0);
					rst = m_argument->getSamplerLocation("triSampler", m_triSamplerLoc);
					rst = m_argument->getTextureLocation("triTexture", m_triTextureLoc);
					rst = m_argument->getUniformBlockLocation("Argument1", m_triTransformLoc);
					m_argument->bindSampler(m_triSamplerLoc, m_triSampler);
					m_argument->bindTexture(m_triTextureLoc, m_computeOutput);
					m_argument->bindUniformBuffer(m_triTransformLoc, m_triTransformMatrix);
				}
				{ // renderable
					m_renderable = m_material->createRenderable();
					m_vertexBuffer = m_context->createVertexBuffer(PlaneVertices, sizeof(PlaneVertices));
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
			m_mainRenderPass->setViewport(vp);
			m_mainRenderPass->setScissor(ss);
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

				Nix::ComputeCommand comp;
				comp.argument = m_computeArgument;
				comp.constantSize = 0;
				comp.constants = nullptr;
				comp.groupX = 32;
				comp.groupY = 32;
				comp.groupZ = 1;
				comp.computePipeline = m_computePipeline;
				comp.outputImage = m_computeOutput;
				comp.outputSSBO = nullptr;

				m_context->executeCompute(comp);

				m_mainRenderPass->begin(m_primQueue); {
					glm::mat4 transMat = glm::mat4();
					m_argument->updateUniformBuffer(m_triTransformMatrix, &transMat, 0, 64);
					m_mainRenderPass->bindArgument(m_argument);
					m_mainRenderPass->bindPipeline(m_pipeline);
					m_mainRenderPass->drawElements(m_renderable, 0, 6);
				}
				m_mainRenderPass->end();

				m_context->endFrame();
			}
		}

		virtual const char * title() {
			return "Triangle ( compute shader )";
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
