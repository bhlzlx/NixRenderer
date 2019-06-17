#include "HDR.h"

#ifdef _WIN32
    #include <Windows.h>
#endif

namespace Nix {

	bool HDR::initialize(void* _wnd, Nix::IArchieve* _archieve ) {
		printf("%s", "HDR is initializing!");

		HMODULE library = ::LoadLibraryA("NixVulkan.dll");
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
		{
			// renderable
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
			//m_indexBuffer = m_context->createIndexBuffer(arrangedIndicesBuffer.data(), arrangedIndicesBuffer.size() * sizeof(uint16_t));
			//m_indexCount = arrangedIndicesBuffer.size();
			m_vertPosition = m_context->createStaticVertexBuffer(positionData.data(), positionData.size() * sizeof(glm::vec3));
			m_vertNormal = m_context->createStaticVertexBuffer(normalData.data(), sizeof(glm::vec3) * normalData.size());
			m_vertCount = positionData.size();
		}

		// =============== create depth pass resources ===============
		{
			MaterialDescription depthMtlDesc;
			Nix::TextReader mtlReader;
			mtlReader.openFile(_archieve, "material/depthOnly.json");
			depthMtlDesc.parse(mtlReader.getText());
			m_mtlDp = m_context->createMaterial(depthMtlDesc);
			RenderPassDescription depthPassDesc;
			Nix::TextReader rpReader;
			rpReader.openFile(_archieve, "renderpass/depthOnly.json");
			m_renderPassDpDesc.parse(rpReader.getText());
			m_pipelineDp = m_mtlDp->createPipeline(m_renderPassDpDesc);
			m_argDp = m_mtlDp->createArgument(0);
			m_renderableDp = m_mtlDp->createRenderable();
			m_renderableDp->setVertexBuffer(m_vertPosition, 0, 0);
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
		//m_lights = { glm::vec3(config.light[0], config.light[1], config.light[2]) };

		return true;
	}

	void HDR::resize(uint32_t _width, uint32_t _height) {
		printf("resized!");
		_width = (_width + 3) & ~(3);
		_height = (_height + 3) & ~(3);
		m_context->resize(_width, _height);

		Viewport vp = {
			0.0f, 0.0f, (float)_width, (float)_height, 0.0f, 1.0f
		};
		Scissor ss;
		ss.origin = { 0, 0 };
		ss.size = { (int)_width, (int)_height };
		//
		m_projection = glm::perspective<float>(90, (float)_width / (float)_height, 0.01f, 400.0f);
		if (m_depthDp) {
			m_depthDp->release();
			m_depthDp = nullptr;
		}
		m_depthDp = m_context->createAttachment(m_renderPassDpDesc.depthStencil.format, _width, _height);
		m_renderPassDp = m_context->createRenderPass(m_renderPassDpDesc, nullptr, m_depthDp);
		Nix::RpClear dpClear;
		dpClear.depth = 1.0f;
		m_renderPassDp->setClear(dpClear);
		//
		m_pipelineDp->setViewport(vp);
		m_pipelineDp->setScissor(ss);
	}

	void HDR::release() {
		printf("destroyed");
		m_context->release();
	}

	void HDR::tick() {
		static float angle = 0.0f;
		angle += 0.01;
		m_model = glm::rotate<float>(glm::mat4(), angle / 180.0f * 3.1415926, glm::vec3(0,1,0));
		glm::mat4 mvp = m_projection * m_view * m_model;

		if (m_context->beginFrame()) 
		{
			m_mainRenderPass->begin(m_primQueue); {
				// 				glm::mat4x4 identity;
				// 				m_argCommon->setUniform(m_matGlobal, 0, &m_projection, 64);
				// 				m_argCommon->setUniform(m_matGlobal, 64, &m_view, 64);
				// 				m_argCommon->setUniform(m_matGlobal, 128, &m_light, sizeof(m_light));
				// 				m_argInstance->setUniform(m_matLocal, 0, &m_model, 64);
				// 				//
				// 				m_mainRenderPass->bindPipeline(m_pipeline);
				// 				m_mainRenderPass->bindArgument(m_argCommon);
				// 				m_mainRenderPass->bindArgument(m_argInstance);
				// 				//					
				// 				m_mainRenderPass->draw(m_renderable, 0, m_vertCount);
				// 				//m_mainRenderPass->drawElements( m_renderable, 0, m_indexCount);
				// 			}
			}

			m_mainRenderPass->end();

			if (m_renderPassDp) {
				m_renderPassDp->begin(m_primQueue); {
					m_argDp->setUniform(0, 0, &mvp, sizeof(mvp));
					m_renderPassDp->bindArgument(m_argDp);
					m_renderPassDp->bindPipeline(m_pipelineDp);
					m_renderPassDp->draw(m_renderableDp, 0, m_vertCount);
					m_renderPassDp->end();
				}
			}

			m_context->endFrame();
		}			
	}

	const char * HDR::title() {
		return "HDR";
	}

	uint32_t HDR::rendererType() {
		return 0;
	}
}

Nix::HDR theapp;

NixApplication* GetApplication() {
    return &theapp;
}