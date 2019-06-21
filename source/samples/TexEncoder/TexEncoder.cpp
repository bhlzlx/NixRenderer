#include <NixApplication.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <NixJpDecl.h>
#include <NixRenderer.h>
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

	struct alignas(16) DXT5Block {
		//
		uint64_t m_alpha;
		uint64_t m_color;

		inline uint8_t calulateAlphaLevel(uint8_t _min, uint8_t _max, uint8_t _alpha) {
			if (_min != _max) {
				uint8_t level = ((_max - _alpha) * 0xff / (_max - _min)) >> 5;
				// 0 1 2 3 4 5 6 7
				// 0 2 3 4 5 6 7 1
				static const uint8_t map[] = { 0,2,3,4,5,6,7,1};
				return map[level];
			} else {
				if (_min) return 0x7;
				else return 0x6;
			}
		}
		inline uint32_t calculateRelativeWeight(uint32_t _color1, uint32_t _color2) {
			uint32_t weight = 0;
			for (int i = 0; i < 3; ++i) {
				uint8_t channel1 = (_color1 >> i * 8) & 0xff;
				uint8_t channel2 = (_color2 >> i * 8) & 0xff;
				if (channel1 > channel2){
					weight += channel1 - channel2;
				} else {
					weight += channel2 - channel1;
				}				
			}
			return weight;
		}
		inline uint8_t calulateColorLevel(uint32_t(&_colorTable)[4], uint32_t _color) {
			uint8_t level = 0;
			uint32_t weight = 0xffffffff;
			for (uint32_t i = 0; i < 4; ++i) {
				uint32_t currentWeight = calculateRelativeWeight(_colorTable[i], _color);
				if (weight > currentWeight) {
					weight = currentWeight;
					level = i;
				}
			}
			return level;
		}

		void compressBitmap(uint32_t* _bitmapData, uint32_t _width, uint32_t _height) {
			m_alpha = m_color = 0;
			uint32_t pixelCount = _width * _height; assert(pixelCount == 16);

			uint32_t alphaMin = -1;	uint32_t alphaMax = 0;
			uint32_t redMin = -1;	uint32_t redMax = 0;
			uint32_t greenMin = -1;	uint32_t greenMax = 0;
			uint32_t blueMin = -1;	uint32_t blueMax = 0;
			// find the max & min value of r g b a
			for (uint32_t i = 0; i < pixelCount; ++i) {
				uint32_t pixel = _bitmapData[i];
				if (alphaMin > (pixel & 0xff000000) ) alphaMin = (pixel & 0xff000000);
				if (alphaMax < (pixel & 0xff000000)) alphaMax = (pixel & 0xff000000);
				if (blueMin > (pixel & 0x00ff0000)) blueMin = (pixel & 0x00ff0000);
				if (blueMax < (pixel & 0x00ff0000)) blueMax = (pixel & 0x00ff0000);
				if (greenMin > (pixel & 0x0000ff00)) greenMin = (pixel & 0x0000ff00);
				if (greenMax < (pixel & 0x0000ff00)) greenMax = (pixel & 0x0000ff00);
				if (redMin > (pixel & 0x000000ff)) redMin = (pixel & 0x000000ff);
				if (redMax < (pixel & 0x000000ff)) redMax = (pixel & 0x000000ff);
			}
			alphaMin >>= 24; alphaMax >>= 24;
			blueMin >>= 16; blueMax >>= 16;
			greenMin >>= 8; greenMax >>= 8;
			//redMin >>= 3; redMax >>= 3;
			//uint64_t colorMax = redMax >> 3 | ((greenMax >> 2) << 5) | ((blueMax >> 3) << 11); // RGB565
			//uint64_t colorMin = redMin >> 3 | ((greenMin >> 2) << 5) | ((blueMin >> 3) << 11); // RGB565
			uint64_t colorMax = blueMax >> 3 | ((greenMax >> 2) << 5) | ((redMax >> 3) << 11); // RGB565
			uint64_t colorMin = blueMin >> 3 | ((greenMin >> 2) << 5) | ((redMin >> 3) << 11); // RGB565

			m_color |= (colorMax | colorMin << 16);

			uint32_t colorTable[4] = {
				redMax | greenMax << 8 | blueMax << 16,
				redMin | greenMin << 8 | blueMin << 16,
				(redMax - (redMax - redMin) / 3) | (greenMax - (greenMax - greenMin) / 3) << 8 | (blueMax - (blueMax - blueMin) / 3) << 16,
				(redMin + (redMax - redMin) / 3) | (greenMin + (greenMax - greenMin) / 3) << 8 | (blueMin + (blueMax - blueMin) / 3) << 16,
			};

			//if (alphaMax == alphaMin) {
//			alphaMax = 0xff;
			//alphaMin = 0x0;
			//}
			m_alpha |= alphaMax;
			m_alpha |= (alphaMin << 8);
			// == ´¦Àí alpha / color ==
			// 16 ¸öÏñËØ
			for (uint32_t pixelIndex = 0; pixelIndex < 16; ++pixelIndex) {
				m_alpha |= (uint64_t)calulateAlphaLevel(alphaMin, alphaMax, _bitmapData[pixelIndex] >> 24) << (16 + pixelIndex * 3);
				m_color |= (uint64_t)calulateColorLevel(colorTable, _bitmapData[pixelIndex]) << (32 + pixelIndex * 2);
			}
		}
	};

	float PlaneVertices[] = {
		-0.5f, -0.5f, 0.0f, 0.0f, 1.0f,
		0.5f, -0.5f, 0.0f, 1.0f, 1.0f,
		0.5f, 0.5f, 0.0f, 1.0f, 0.0f,
		-0.5f, 0.5f, 0.0f, 0.0f, 0.0f,
	};

	uint16_t PlaneIndices[] = {
		0,1,2,0,2,3
	};

	std::vector< DXT5Block > vecDXT5Data;

	class TexEncoder : public NixApplication {
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
			printf("%s", "TexEncoder is initializing!");
#ifdef NDEBUG
			HMODULE library = ::LoadLibraryA("NixVulkan.dll");
#else
			HMODULE library = ::LoadLibraryA("NixVulkand.dll");
#endif
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
			Nix::TextReader mtlReader;
			mtlReader.openFile(_archieve, "material/triangle.json");
			mtlDesc.parse(mtlReader.getText());
			RenderPassDescription rpDesc;
			Nix::TextReader rpReader;
			rpReader.openFile(_archieve, "renderpass/swapchain.json");
			rpDesc.parse(rpReader.getText());
			rpDesc.colors[0].format = m_context->swapchainColorFormat();
			rpDesc.depthStencil.format = m_driver->selectDepthFormat(true);

			/*FILE* file = fopen("d:/fish.png", "r");*/
			int width, height, channel;
			stbi_uc * bytes = stbi_load("E:/Github/NixRenderer/bin/texture/fish.png", &width, &height, &channel, 4);
			uint32_t * pixels = (uint32_t*)bytes;

			

			for (uint32_t col = 0; col < 16; ++col ) {
				for ( uint32_t row = 0; row < 16; ++row ) {
					std::vector<uint32_t> blockData;
					for (uint32_t C = 0; C < 4; ++C ) {
						for (uint32_t R = 0; R < 4; ++R) {
							uint32_t realR = row * 4 + R;
							uint32_t realC = col * 4 + C;
							blockData.push_back(*(pixels + realC * 64 + realR));
						}
					}
					DXT5Block block;
					block.compressBitmap(blockData.data(), 4, 4);
					vecDXT5Data.push_back(block);
				}
			}


			TextureDescription texDesc;
			texDesc.depth = 1;
			texDesc.format = NixBC3_LINEAR_RGBA;
			texDesc.height = 64;
			texDesc.width = 64;
			texDesc.mipmapLevel = 1;
			texDesc.type = Nix::TextureType::Texture2D;

			//Nix::IFile * texFile = _archieve->open("texture/texture_bc3.ktx");
			//Nix::IFile * texFile = _archieve->open("texture/texture_array_bc3.ktx");
			//Nix::IFile* texMem = CreateMemoryBuffer(texFile->size());
			//texFile->read(texFile->size(), texMem);
			m_texture = m_context->createTexture(texDesc);
			//m_texture = m_context->createTextureKTX(texMem->constData(), texMem->size());

			TextureRegion region;
			region.baseLayer = 0;
			region.mipLevel = 0;
			region.offset.x = region.offset.y = region.offset.z = 0.0f;
			region.size.depth = 1; region.size.height = 64; region.size.width = 64;

			m_texture->setSubData(vecDXT5Data.data(), vecDXT5Data.size() * sizeof(DXT5Block), region);
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
			return "TexEncoder (with Texture2D Array)";
		}

		virtual uint32_t rendererType() {
			return 0;
		}
	};
}

Nix::TexEncoder theapp;

NixApplication* GetApplication() {
    return &theapp;
}