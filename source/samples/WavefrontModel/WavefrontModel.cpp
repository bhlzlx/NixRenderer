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


		std::vector<IRenderable*>	m_vecRenderable;
		std::vector<uint32_t>		m_vecIndicesCount;
		std::vector<IBuffer*>		m_vertices;
		std::vector<IBuffer*>		m_colors;
		std::vector<IBuffer*>		m_indices;
		//
		IPipeline*					m_pipeline;

		glm::mat4					m_model;
		glm::mat4					m_view;
		glm::mat4					m_projection;

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
					objReaderConfig.vertex_color = true;
					const std::string objText(objTextReader.getText());
					const std::string mtlText(mtlTextReader.getText());
					readRst = objReader.ParseFromString(objText, mtlText, objReaderConfig);
					assert(readRst);

					auto vertexBuffer = m_context->createStaticVertexBuffer(objReader.GetAttrib().vertices.data(), sizeof(float) * objReader.GetAttrib().vertices.size());
					auto colorBuffer = m_context->createStaticVertexBuffer(objReader.GetAttrib().colors.data(), sizeof(float) * objReader.GetAttrib().colors.size());
					m_vertices.push_back(vertexBuffer);
					m_colors.push_back(colorBuffer);
					const auto& shapes = objReader.GetShapes();
					for (auto& shape : shapes) {
						auto renderable = m_material->createRenderable();
						m_vecRenderable.push_back(renderable);
						std::vector<uint16_t> indices;
						for (auto& index : shape.mesh.indices) {
							indices.push_back((uint16_t)index.vertex_index);
						}
						auto indexBuffer = m_context->createIndexBuffer(indices.data(), indices.size() * sizeof(uint16_t));
						m_indices.push_back(indexBuffer);
						m_vecIndicesCount.push_back((uint32_t)indices.size());
						renderable->setIndexBuffer(indexBuffer, 0);
						renderable->setVertexBuffer(colorBuffer, 0, 1);
						renderable->setVertexBuffer(vertexBuffer, 0, 0);
					}
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
			m_projection = glm::perspective<float>(90, (float)_width / (float)_height, 0.01f, 200.0f);
			m_view = glm::lookAt(glm::vec3(-80, 40 , -80), glm::vec3(0, 0, 0), glm::vec3(0,1,0));
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
					m_argCommon->setUniform(m_matGlobal, 0, &m_projection, 64);
					m_argCommon->setUniform(m_matGlobal, 64, &m_view, 64);
					m_argInstance->setUniform(m_matLocal, 0, &identity, 64);
					//
					m_mainRenderPass->bindPipeline(m_pipeline);
					m_mainRenderPass->bindArgument(m_argCommon);
					m_mainRenderPass->bindArgument(m_argInstance);
					//
					for (uint32_t i = 0; i < m_vecRenderable.size(); ++i) {
						m_mainRenderPass->drawElements( m_vecRenderable[i], 0, m_vecIndicesCount[i]);
					}
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