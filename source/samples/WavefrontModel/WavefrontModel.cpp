#include <NixApplication.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <NixJpDecl.h>
#include <NixRenderer.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <tiny_obj_loader.h>

#include <nix/io/archieve.h>
#include <cstdio>
#include <cassert>

#ifdef _WIN32
    #include <Windows.h>
#endif

namespace Nix {

	struct Config {
		float eye[3];
		float lookat[3];
		float up[3];
		float light[3];
		NIX_JSON(eye, lookat, up, light)
	};

	class WavefrontModel : public NixApplication {
	private:
		IArchieve*					m_arch;
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
		
		IBuffer*					m_indexBuffer;
		uint32_t					m_indexCount;
		IBuffer*					m_vertPosition;
		uint32_t					m_vertCount;
		IBuffer*					m_vertNormal;
		//
		IPipeline*					m_pipeline;

		glm::mat4					m_model;
		glm::mat4					m_view;
		glm::mat4					m_projection;
		glm::vec3					m_light;

		//
		virtual bool initialize(void* _wnd, Nix::IArchieve* _archieve ) {
			printf("%s", "WavefrontModel is initializing!");

			HMODULE library = ::LoadLibrary("NixVulkan.dll");
			assert(library);

			typedef IDriver*(* PFN_CREATE_DRIVER )();

			PFN_CREATE_DRIVER createDriver = reinterpret_cast<PFN_CREATE_DRIVER>(::GetProcAddress(library, "createDriver"));

			// \ 1. create driver
			m_arch = _archieve;
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
			mtlReader.openFile(_archieve, "material/lowpoly.json");
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

			bool rst = false;
			m_material = m_context->createMaterial(mtlDesc); {
				{ // graphics pipeline 
					m_pipeline = m_material->createPipeline(rpDesc);
				}
				{ // arguments
					m_argCommon = m_material->createArgument(0);
					rst = m_argCommon->getUniformBlock("GlobalArgument", &m_matGlobal);
					m_argInstance = m_material->createArgument(1);
					rst = m_argInstance->getUniformBlock("LocalArgument", &m_matLocal);
				}
				{ // renderable
					Nix::TextReader objTextReader;
					bool readRst = objTextReader.openFile(m_arch, "model/Low_Poly_Mill/low-poly-mill.obj");
					assert(readRst);
					Nix::TextReader mtlTextReader;
					readRst = mtlTextReader.openFile(m_arch, "model/Low_Poly_Mill/low-poly-mill.mtl");
					assert(readRst);
					tinyobj::ObjReader objReader;
					tinyobj::ObjReaderConfig objReaderConfig;

					objReaderConfig.triangulate = true;
					const std::string objText(objTextReader.getText());
					const std::string mtlText(mtlTextReader.getText());
					readRst = objReader.ParseFromString(objText, mtlText, objReaderConfig);
					assert(readRst);

					std::vector< glm::vec3 > positionData;
					std::vector< glm::vec3 > normalData;
		
					const auto& shapes = objReader.GetShapes();
					for (auto& shape : shapes) {
						for (auto& index : shape.mesh.indices) {
							// arrange the normal attribute
							positionData.push_back(
								glm::vec3(
									objReader.GetAttrib().vertices[0 + index.vertex_index * 3],
									objReader.GetAttrib().vertices[1 + index.vertex_index * 3],
									objReader.GetAttrib().vertices[2 + index.vertex_index * 3]
								)
							);
						}
					}

					for (uint32_t triangleIndex = 0; triangleIndex < positionData.size() / 3; ++triangleIndex) {
						glm::vec3 vec1 = positionData[triangleIndex * 3 + 2] - positionData[triangleIndex * 3 + 1];
						glm::vec3 vec2 = positionData[triangleIndex * 3 + 1] - positionData[triangleIndex * 3];
						glm::vec3 normal = glm::cross(vec1, vec2);
						normalData.push_back(normal);
						normalData.push_back(normal);
						normalData.push_back(normal);
					}


					m_renderable = m_material->createRenderable();
					//m_indexBuffer = m_context->createIndexBuffer(arrangedIndicesBuffer.data(), arrangedIndicesBuffer.size() * sizeof(uint16_t));
					//m_indexCount = arrangedIndicesBuffer.size();
					m_vertPosition = m_context->createStaticVertexBuffer( positionData.data(), positionData.size() * sizeof(glm::vec3) );
					m_vertNormal = m_context->createStaticVertexBuffer(normalData.data(), sizeof(glm::vec3) * normalData.size());
					m_vertCount = positionData.size();
					m_renderable->setVertexBuffer( m_vertNormal, 0, 1);
					m_renderable->setVertexBuffer( m_vertPosition, 0, 0);
				}
			}

			Config config;
			{
				Nix::TextReader cfgReader;
				cfgReader.openFile(_archieve, "config/lowpoly.json");
				config.parse(cfgReader.getText());
			}
			m_view = glm::lookAt(
				glm::vec3(config.eye[0], config.eye[1], config.eye[2]),
				glm::vec3(config.lookat[0], config.lookat[1], config.lookat[2]),
				glm::vec3(config.up[0], config.up[1], config.up[2])
			);
			m_light = glm::vec3(config.light[0], config.light[1], config.light[2]);

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
			m_projection = glm::perspective<float>(90, (float)_width / (float)_height, 0.01f, 400.0f);
			m_pipeline->setViewport(vp);
			m_pipeline->setScissor(ss);
		}

		virtual void release() {
			printf("destroyed");
			m_context->release();
		}

		virtual void tick() {
			static float angle = 0.0f;
			angle += 0.01;
			m_model = glm::rotate<float>(glm::mat4(), angle / 180.0f * 3.1415926, glm::vec3(0,1,0));
			if (m_context->beginFrame()) {
				m_mainRenderPass->begin(m_primQueue); {
					glm::mat4x4 identity;
					m_argCommon->setUniform(m_matGlobal, 0, &m_projection, 64);
					m_argCommon->setUniform(m_matGlobal, 64, &m_view, 64);
					m_argCommon->setUniform(m_matGlobal, 128, &m_light, sizeof(m_light));
					m_argInstance->setUniform(m_matLocal, 0, &m_model, 64);
					//
					m_mainRenderPass->bindPipeline(m_pipeline);
					m_mainRenderPass->bindArgument(m_argCommon);
					m_mainRenderPass->bindArgument(m_argInstance);
					//					
					m_mainRenderPass->draw(m_renderable, 0, m_vertCount);
					//m_mainRenderPass->drawElements( m_renderable, 0, m_indexCount);
				}
				m_mainRenderPass->end();

				m_context->endFrame();
			}			
		}

		virtual const char * title() {
			return "WavefrontModel";
		}

		virtual uint32_t rendererType() {
			return 0;
		}
	};
}

Nix::WavefrontModel theapp;

NixApplication* GetApplication() {
    return &theapp;
}