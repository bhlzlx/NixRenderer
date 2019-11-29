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
#include <glm/glm.hpp>

#ifdef _WIN32
#include <Windows.h>
#endif
#include "../FreeCamera.h"

namespace Nix {

	/*==============
		3 --- 2
		|     |
		0 --- 1	
	==============*/

	struct Patch {
		float x, y, z;
		float u, v;
	};

	const Patch blockPatch[4] = {
		// pos(x,z) - uv
		{ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
		{ 1.0f, 0.0f, 0.0f, 1.0f, 0.0f },
		{ 1.0f, 0.0f, 1.0f, 1.0f, 1.0f },
		{ 0.0f, 0.0f, 1.0f, 0.0f, 1.0f },
	};

	uint16_t oneBlockPatchIndices[] = {
		0, 1, 2, 3
	};

	Patch testPatches[9][4];
	uint16_t testPatchIndices[36];

	void FillPatch( Patch* _ptr, int _x, int _y) {
		memcpy(_ptr, blockPatch, sizeof(blockPatch));
		for (uint32_t i = 0; i < 4; ++i) {
			_ptr[i].x += _x;
			_ptr[i].z += _y;
		}
		if (_x & 1) {
			std::swap(_ptr[0].u, _ptr[1].u);
			std::swap(_ptr[2].u, _ptr[3].u);
		}
		if (_y & 1) {
			std::swap(_ptr[0].v, _ptr[3].v);
			std::swap(_ptr[2].v, _ptr[1].v);
		}
	}

	

	const float perspectiveNear = 0.1f;
	const float perspectiveFar = 512.0f;
	const float perspectiveFOV = 3.1415926f / 2;
	const float particleSize = 2.0f;

	ITexture* LoadTextureFromFileSTB( TextureDescription& _desc, uint32_t _channel, const std::string& _filepath, IContext* _context, IArchieve* _arch ) {
		ITexture* tex = nullptr;
		tex = _context->createTexture(_desc);
		Nix::IFile* fishPNG = _arch->open(_filepath);
		Nix::IFile* memPNG = CreateMemoryBuffer(fishPNG->size());
		fishPNG->read(fishPNG->size(), memPNG);

		int x, y, channel;
		auto rawData = stbi_load_from_memory((const stbi_uc*)memPNG->constData(), memPNG->size(), &x, &y, &channel, _channel);
		BufferImageUpload upload;
		upload.baseMipRegion = {
			0, 0,{ 0 , 0 , 0 },{ (uint32_t)x, (uint32_t)y, 1 }
		};
		upload.data = rawData;
		upload.length = x * y * _channel;
		upload.mipCount = 1;
		upload.mipDataOffsets[0] = 0;
		tex->uploadSubData(upload);
		stbi_image_free(rawData);
		return tex;
	}

	class Triangle : public NixApplication {
	private:
		IDriver*					m_driver;
		IContext*					m_context;
		IRenderPass*				m_mainRenderPass;
		IGraphicsQueue*				m_primQueue;
		//
		IMaterial*					m_material;

		IArgument*					m_argument;
		//
		uint32_t					m_TCSArgumentSlot;
		IBuffer*					m_TCSArgumentUniform;
		uint32_t					m_TESArgumentSlot;
		IBuffer*					m_TESArgumentUniform;

		uint32_t					m_TESTextureSlot;
		ITexture*					m_TESTexture;

		uint32_t					m_TESSamplerSlot;
		//
		IRenderable*				m_renderable;
		IBuffer*					m_vertexBuffer;
		IBuffer*					m_indexBuffer;

		ITexture*					m_grassTexture;
		ITexture*					m_sandTexture;
		ITexture*					m_snowTexture;

		IPipeline*					m_pipeline;

		FreeCamera					m_camera;
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
			mtlReader.openFile(_archieve, "material/terrain_tessellation.json");
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
			texDesc.height = 64;
			texDesc.width = 64;
			texDesc.mipmapLevel = 1;
			texDesc.type = Nix::TextureType::Texture2D;
			m_TESTexture = LoadTextureFromFileSTB(texDesc, 1, "texture/terrian.png", m_context, _archieve);
			texDesc.format = NixRGBA8888_UNORM;
			texDesc.width = texDesc.height = 512;
			m_grassTexture = LoadTextureFromFileSTB(texDesc, 4, "texture/grass.png", m_context, _archieve);
			m_snowTexture = LoadTextureFromFileSTB(texDesc, 4, "texture/snow.png", m_context, _archieve);
			m_sandTexture = LoadTextureFromFileSTB(texDesc, 4, "texture/sand.png", m_context, _archieve);

			m_TESArgumentUniform = m_context->createUniformBuffer(128);
			m_TCSArgumentUniform = m_context->createUniformBuffer(128);


			bool rst = false;
			m_material = m_context->createMaterial(mtlDesc); {
				{ // graphics pipeline 
					m_pipeline = m_material->createPipeline(rpDesc);
				}
				{ // arguments
					m_argument = m_material->createArgument(0);
					m_argument->getUniformBlockLocation("TCSArgument", m_TCSArgumentSlot);
					m_argument->getUniformBlockLocation("TESArgument", m_TESArgumentSlot);

					m_argument->getSamplerLocation("terrainSampler", m_TESSamplerSlot);

					m_argument->getTextureLocation("terrainTexture", m_TESTextureSlot);
					//
					uint32_t colorSamplerSlot;
					m_argument->getSamplerLocation("texSampler", colorSamplerSlot);
					uint32_t grassSlot;
					m_argument->getTextureLocation("texGrass", grassSlot);
					uint32_t sandSlot;
					m_argument->getTextureLocation("texSand", sandSlot);
					uint32_t snowSlot;
					m_argument->getTextureLocation("texSnow", snowSlot);
					//
					SamplerState TESSamplerState;
					TESSamplerState.min = TextureFilter::TexFilterLinear;
					TESSamplerState.mag = TextureFilter::TexFilterLinear;
					m_argument->bindSampler(m_TESSamplerSlot, TESSamplerState);
					m_argument->bindTexture(m_TESTextureSlot, m_TESTexture);
					m_argument->bindUniformBuffer(m_TCSArgumentSlot, m_TCSArgumentUniform);
					m_argument->bindUniformBuffer(m_TESArgumentSlot, m_TESArgumentUniform);


					m_argument->bindSampler(colorSamplerSlot, TESSamplerState);
					m_argument->bindTexture(grassSlot, m_grassTexture);
					m_argument->bindTexture(snowSlot, m_snowTexture);
					m_argument->bindTexture(sandSlot, m_sandTexture);

					//m_argInstance = m_material->createArgument(1);
					//rst = m_argInstance->getUniformBlock("LocalArgument", &m_matLocal);
				}
				{ // renderable
					for (uint32_t x = 0; x < 3; ++x) {
						for (uint32_t y = 0; y < 3; ++y) {
							FillPatch(&testPatches[x*3+y][0], x, y);							
						}
					}
					for (uint32_t n = 0; n < 36; ++n) {
						testPatchIndices[n] = n;
					}
					m_renderable = m_material->createRenderable();
					m_vertexBuffer = m_context->createVertexBuffer(testPatches, sizeof(testPatches));
					m_indexBuffer = m_context->createIndexBuffer(testPatchIndices, sizeof(testPatchIndices));
					//m_vertexBuffer = m_context->createVertexBuffer(nullptr, sizeof(PlaneVertices));
					//m_indexBuffer = m_context->createIndexBuffer(nullptr, sizeof(PlaneIndices));
					m_renderable->setVertexBuffer(m_vertexBuffer, 0, 0);
					m_renderable->setIndexBuffer(m_indexBuffer, 0);
				}
			}

			m_camera.SetEye(glm::vec3(0, 4, 0));

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

			m_camera.Perspective(perspectiveFOV, (float)_width / (float)_height, perspectiveNear, perspectiveFar);
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

			m_camera.Tick();

			glm::vec3 eye = m_camera.GetEye();

			float imageIndex = (tickCounter / 1024) % 4;

			if (m_context->beginFrame()) {
				m_mainRenderPass->begin(m_primQueue); {
					//
					struct {
						glm::mat4 tcsModel;
						glm::vec3 eye;
					} tcsTrans;
					tcsTrans.eye = eye;
					tcsTrans.tcsModel = glm::scale<float>(glm::mat4(), glm::vec3(64.0f, 1.0f, 64.0f));
					glm::mat4 tesTrans = m_camera.GetProjMatrix() * m_camera.GetViewMatrix() * tcsTrans.tcsModel;
					m_argument->updateUniformBuffer(m_TCSArgumentUniform, &tcsTrans, 0, 128);
					m_argument->updateUniformBuffer(m_TESArgumentUniform, &tesTrans, 0, 64);
					//
					m_mainRenderPass->bindPipeline(m_pipeline);
					m_mainRenderPass->bindArgument(m_argument);
					//m_mainRenderPass->bindArgument(m_argInstance);
					//
					m_mainRenderPass->drawElements(m_renderable, 0, 36);
				}
				m_mainRenderPass->end();

				m_context->endFrame();
			}
		}

		virtual const char * title() {
			return "Terrain Mesh ( with Tessellation shader )";
		}

		virtual uint32_t rendererType() {
			return 0;
		}


		void onMouseEvent(eMouseButton _bt, eMouseEvent _event, int _x, int _y)
		{
			static int lx = 0;
			static int ly = 0;

			static int dx;
			static int dy;

			switch (_event)
			{
			case MouseDown:
			{
				if (_bt == RButtonMouse)
				{
					lx = _x;
					ly = _y;
				}
				break;
			}
			case MouseUp:
			{
				break;
			}
			case MouseMove:
			{
				if (_bt == RButtonMouse)
				{
					dx = _x - lx;
					dy = _y - ly;
					ly = _y;
					lx = _x;
					m_camera.RotateAxisY((float)dx / 10.0f * 3.1415f);
					m_camera.RotateAxisX((float)dy / 10.0f * 3.1415f);
				}
				break;
			}
			}
		}

		void onKeyEvent(unsigned char _key, eKeyEvent _event)
		{
			if (_event == NixApplication::eKeyDown)
			{
				switch (_key)
				{
				case 'W':
				case 'w': {
					//auto forward = m_camera.GetForward();
					m_camera.Forward(1.0f);
					break;
				}
				case 'a':
				case 'A': {
					m_camera.Leftward(1.0);
					//auto leftward = m_camera.GetLeftward();
					//m_camera.Leftward(cammeraStepDistance);
					break;
				}
				case 's':
				case 'S':
				{
					m_camera.Forward(-1.0);
					//auto rightward = -m_camera.GetLeftward();
					//m_camera.Backward(cammeraStepDistance);
					break;
				}
				case 'd':
				case 'D': {
					m_camera.Leftward(-1.0);
					//auto backward = -m_camera.GetForward();
					//moveDirection = PxVec3(backward.x, backward.y, backward.z);
					//m_camera.Rightward(cammeraStepDistance);
					break;
				}
				}
			}
		}
	};
}

Nix::Triangle theapp;

NixApplication* GetApplication() {
	return &theapp;
}
