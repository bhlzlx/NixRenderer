#include "HDR.h"
#include <math.h>

#ifdef _WIN32
    #include <Windows.h>
#endif

namespace Nix {

	float vertRect[] = {
		-1.0f, -1.0f, 0.0f, 0.0f, 1.0f,
		1.0f, -1.0f, 0.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 0.0f, 1.0f, 0.0f,
		-1.0f, 1.0f, 0.0f, 0.0f, 0.0f
	};

	uint16_t indcRect[] = {
		0, 1, 2, 2, 3, 0
	};

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

		// =============== create color pass resources ===============
		{
			MaterialDescription materialDesc;
			Nix::TextReader mtlReader;
			mtlReader.openFile(_archieve, "material/early-z.json");
			materialDesc.parse(mtlReader.getText());
			m_mtlEarlyZ = m_context->createMaterial(materialDesc);

			m_argEarlyZ = m_mtlEarlyZ->createArgument(0);
			m_renderableEarlyZ = m_mtlEarlyZ->createRenderable();
			m_renderableEarlyZ->setVertexBuffer(m_vertPosition, 0, 0);
			m_renderableEarlyZ->setVertexBuffer(m_vertNormal, 0, 1);
			//
			Nix::TextReader rpReader;
			rpReader.openFile(_archieve, "renderpass/HDR.json");
			m_renderPassEarlyZDesc.parse(rpReader.getText());
			m_pipelineEarlyZ = m_mtlEarlyZ->createPipeline(m_renderPassEarlyZDesc);
		}

		// =============== tone mapping resources ===============
		{
			MaterialDescription materialDesc;
			Nix::TextReader mtlReader;
			mtlReader.openFile(_archieve, "material/tonemapping.json");
			materialDesc.parse(mtlReader.getText());
			m_mtlToneMapping = m_context->createMaterial(materialDesc);

			m_argToneMapping = m_mtlToneMapping->createArgument(0);
			m_renderableToneMapping = m_mtlToneMapping->createRenderable();

			m_vboTM = m_context->createStaticVertexBuffer(vertRect, sizeof(vertRect));
			m_iboTM = m_context->createIndexBuffer(indcRect, sizeof(indcRect));

			m_renderableToneMapping->setVertexBuffer(m_vboTM, 0, 0);
			m_renderableToneMapping->setIndexBuffer(m_iboTM, 0);

			Nix::TextReader rpReader;
			rpReader.openFile(_archieve, "renderpass/swapchain.json");
			RenderPassDescription rpDesc;
			rpDesc.parse(rpReader.getText());
			m_pipelineToneMapping = m_mtlToneMapping->createPipeline(rpDesc);

		}



		{
			Nix::TextReader cfgReader;
			cfgReader.openFile(_archieve, "config/lowpoly.json");
			m_config.parse(cfgReader.getText());
		}

		m_view = glm::lookAt(
			glm::vec3(m_config.eye[0], m_config.eye[1], m_config.eye[2]),
			glm::vec3(m_config.lookat[0], m_config.lookat[1], m_config.lookat[2]),
			glm::vec3(m_config.up[0], m_config.up[1], m_config.up[2])
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

		if (m_colorEarlyZ) {
			m_colorEarlyZ->release();
			m_colorEarlyZ = nullptr;
		}
		m_colorEarlyZ = m_context->createAttachment(m_renderPassEarlyZDesc.colors[0].format, _width, _height);
		m_renderPassEarlyZ = m_context->createRenderPass(m_renderPassEarlyZDesc, &m_colorEarlyZ, m_depthDp);

		Nix::RpClear dpClear;
		dpClear.colors[0] = { 0.7f,0.7f ,0.7f ,1.0f };
		dpClear.depth = 1.0f;
		m_renderPassDp->setClear(dpClear);
		m_renderPassEarlyZ->setClear(dpClear);
		//
		m_pipelineDp->setViewport(vp);
		m_pipelineDp->setScissor(ss);
		m_pipelineEarlyZ->setViewport(vp);
		m_pipelineEarlyZ->setScissor(ss);
		m_pipelineToneMapping->setViewport(vp);
		m_pipelineToneMapping->setScissor(ss);

		uint32_t id;
		bool rst = m_argToneMapping->getSampler("hdrImage", &id);
		assert(rst);
		Nix::SamplerState samplerState;
		m_argToneMapping->setSampler(id, samplerState, m_colorEarlyZ->getTexture());
	}

	void HDR::release() {
		printf("destroyed");
		m_context->release();
	}

	void HDR::tick() {
		static float angle = 0.0f;
		angle += 0.1;
		m_model = glm::rotate<float>(glm::mat4(), angle / 180.0f * 3.1415926, glm::vec3(0,1,0));
		glm::mat4 mvp = m_projection * m_view * m_model;

		if (m_context->beginFrame()) 
		{
			// === depth pass ===
			if (m_renderPassDp) {
				m_renderPassDp->begin(m_primQueue); {
					m_argDp->setUniform(0, 0, &mvp, sizeof(mvp));
					m_renderPassDp->bindArgument(m_argDp);
					m_renderPassDp->bindPipeline(m_pipelineDp);
					m_renderPassDp->draw(m_renderableDp, 0, m_vertCount);
					m_renderPassDp->end();
				}
			}
			// === color pass ===
			if (m_renderPassEarlyZ) {
				m_renderPassEarlyZ->begin(m_primQueue);

				float constant = 1.0f;
				float linear = 0.06f;
				float quadratic = 0.01f;
				uint32_t lightCount = 3;
				glm::vec4 lights[] = { 
					glm::vec4(-20.0f, 18.0f, -20.0f, 0.0f), glm::vec4(1.0f, 2.0f, 0.5f,0.0f),
					glm::vec4(-10.0f, 18.0f, -16.0f, 0.0f), glm::vec4(2.0f, 0.5f, 1.0f,0.0f),
					glm::vec4(-10.0f, 18.0f, -0.0f, 0.0f), glm::vec4(0.5f, 1.0f, 2.5f,0.0f),
				};

				m_argEarlyZ->setUniform(0, 0, &m_projection, sizeof(m_projection));
				m_argEarlyZ->setUniform(0, 64, &m_view, sizeof(m_view));
				m_argEarlyZ->setUniform(0, 128, &m_model, sizeof(m_model));

				m_argEarlyZ->setUniform(1, 0, &constant, 4);
				m_argEarlyZ->setUniform(1, 4, &linear, 4);
				m_argEarlyZ->setUniform(1, 8, &quadratic, 4);
				

				m_argEarlyZ->setUniform(1, 12, &lightCount, 4);
				m_argEarlyZ->setUniform(1, 16, &lights, sizeof(lights));


				m_renderPassEarlyZ->bindArgument(m_argEarlyZ);
				m_renderPassEarlyZ->bindPipeline(m_pipelineEarlyZ);
				m_renderPassEarlyZ->draw(m_renderableEarlyZ, 0, m_vertCount);

				m_renderPassEarlyZ->end();
			}

			m_mainRenderPass->begin(m_primQueue); {
				static uint64_t frameCounter = 0;
				++frameCounter;
				float adapt_lum = (float) abs(int(frameCounter % 100) - 50) / 100.0f + 0.1f;
				m_argToneMapping->setUniform(0, 0, &adapt_lum, 4);
				m_mainRenderPass->bindPipeline(m_pipelineToneMapping);
				m_mainRenderPass->bindArgument(m_argToneMapping);
				m_mainRenderPass->drawElements(m_renderableToneMapping, 0, 6);
			}

			m_mainRenderPass->end();

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