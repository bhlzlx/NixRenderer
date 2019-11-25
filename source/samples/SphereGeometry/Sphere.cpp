#define NIX_JP_IMPLEMENTATION
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
#include <NixApplication.h>

#undef GetObject


namespace Nix {

	struct CVertex {
		glm::vec3 xyz;
		glm::vec3 norm;
		glm::vec2 coord;
	};

	struct Config {
		float eye[3];
		float lookat[3];
		float up[3];
		float light[3];
		NIX_JSON( eye, lookat, up, light )
	};

	void CreateSphere(int _nstack, int _nslice, std::vector<CVertex>& _vertices, std::vector<uint16_t>& _indices) {
		assert(_nstack >= 3);
		assert(_nslice >= 3);
		//
		float dy = 2.0f / _nstack;
		int mid = (_nstack - 1) / 2;
		int midSliceCount = mid + _nslice;

		std::vector< CVertex > vertices;

		for (int stk = 0; stk < _nstack; ++stk) {
			int sliceCount = 0;
			if (_nstack & 0x1) { // ÆæÊý
				sliceCount = midSliceCount - abs(mid - stk);
			}
			else {
				if (stk > mid) {
					sliceCount = midSliceCount - abs(mid - stk + 1);
				}
				else {
					sliceCount = midSliceCount - abs(mid - stk);
				}
			}
			float yarc = 3.141592654f / (_nstack - 1) * stk;
			float y = cos(yarc);
			float xzref = sin(yarc);
			float x = 0;
			float z = 0;
			float dxarc = 3.141592654f * 2.0f / (sliceCount - 1);
			for (int slc = 0; slc < sliceCount; ++slc) {
				float xarc = dxarc * slc;
				x = cos(xarc) * xzref;
				z = sin(xarc) * xzref;
				//
				glm::vec3 pos(x, y, z);
				glm::vec3 normal(x, y, z);
				glm::vec2 coord(1.0f - 1.0f / (sliceCount - 1) * slc, 1.0f - (y + 1.0f) / 2.0f);
				//
				CVertex vertex;
				vertex.xyz = pos;
				vertex.norm = normal;
				vertex.coord = coord;
				//
				_vertices.push_back(vertex);
			}
			//printf("stack : %d , slice : %d", stk, sliceCount);
		}
		// create indices
		size_t count = _vertices.size();
		size_t counter = 0;
		if (_nstack & 0x1) { // ÆæÊý
			for (int stk = 0; stk < mid; ++stk) {
				int sliceCount = midSliceCount - abs(mid - stk);
				for (size_t ct = 1; ct <= sliceCount; ++ct)
				{
					uint16_t sixple[6];
					sixple[0] = counter;
					sixple[1] = counter + sliceCount;
					sixple[2] = sixple[1] + 1;
					//
					sixple[3] = (count - sixple[0]) - 1;
					sixple[4] = (count - sixple[1]) - 1;
					sixple[5] = (count - sixple[2]) - 1;
					//
					for (auto index : sixple)
						_indices.push_back(index);
					if (stk != 0 && ct != sliceCount) {
						sixple[0] = counter;
						sixple[1] = counter + sliceCount + 1;
						sixple[2] = counter + 1;

						sixple[3] = (count - sixple[0]) - 1;
						sixple[4] = (count - sixple[1]) - 1;
						sixple[5] = (count - sixple[2]) - 1;


						//sixple[3] = count - sixple[0] - 1; sixple[4] = count - sixple[1] - 1; sixple[5] = sixple[4] - 1;
						for (auto index : sixple)
							_indices.push_back(index);
					}
					//
					++counter;
				}
			}
		}
		else { // Å¼Êý
			for (int stk = 0; stk < mid; ++stk) {
				int sliceCount = 0;
				if (stk > mid) {
					sliceCount = midSliceCount - abs(mid - stk + 1);
				}
				else {
					sliceCount = midSliceCount - abs(mid - stk);
				}
				//
				for (size_t ct = 1; ct <= sliceCount; ++ct)
				{
					uint16_t sixple[6];
					sixple[0] = counter;
					sixple[1] = counter + sliceCount;
					sixple[2] = sixple[1] + 1;
					//
					sixple[3] = (count - sixple[0]) - 1;
					sixple[4] = (count - sixple[1]) - 1;
					sixple[5] = (count - sixple[2]) - 1;
					//
					for (auto index : sixple)
						_indices.push_back(index);
					if (stk != 0 && ct != sliceCount) {
						sixple[0] = counter;
						sixple[1] = counter + sliceCount + 1;
						sixple[2] = counter + 1;

						sixple[3] = (count - sixple[0]) - 1;
						sixple[4] = (count - sixple[1]) - 1;
						sixple[5] = (count - sixple[2]) - 1;


						//sixple[3] = count - sixple[0] - 1; sixple[4] = count - sixple[1] - 1; sixple[5] = sixple[4] - 1;
						for (auto index : sixple)
							_indices.push_back(index);
					}
					//
					++counter;
				}
			}
			//
			for (int ct = 0; ct < midSliceCount - 1; ++ct) {
				uint16_t sixple[6];
				sixple[0] = counter;
				sixple[1] = sixple[0] + midSliceCount;
				sixple[2] = sixple[0] + 1;
				//
				sixple[3] = (count - sixple[0]) - 1;
				sixple[4] = (count - sixple[1]) - 1;
				sixple[5] = (count - sixple[2]) - 1;
				for (auto index : sixple)
					_indices.push_back(index);
				++counter;
			}
		}
	}
	

	class Sphere : public NixApplication {
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
		IBuffer*					m_commonArgUnif;

		IArgument*					m_argInstance;
		uint32_t					m_matLocal;
		IBuffer*					m_instanceArgUnif;

		IRenderable*				m_renderable;
		
		IBuffer*					m_indexBuffer;
		uint32_t					m_indexCount;
		IBuffer*					m_vertexBuffer;
		//
		IPipeline*					m_pipeline;

		glm::mat4					m_model;
		glm::mat4					m_view;
		glm::mat4					m_projection;
		glm::vec3					m_light;

		//
		virtual bool initialize(void* _wnd, Nix::IArchieve* _archieve ) {
			printf("%s", "Sphere is initializing!");

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
			MaterialDescription mtlDesc;
			Nix::TextReader mtlReader;
			mtlReader.openFile(_archieve, "material/sphere.json");
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

			bool rst = false;
			m_material = m_context->createMaterial(mtlDesc); {
				{ // graphics pipeline 
					m_pipeline = m_material->createPipeline(rpDesc);
				}
				{ // arguments
					m_argCommon = m_material->createArgument(0);
					m_argCommon->getUniformBlockLocation("GlobalArgument", m_matGlobal);
					m_argInstance = m_material->createArgument(1);
					rst = m_argInstance->getUniformBlockLocation("LocalArgument", m_matLocal);
					//
					m_commonArgUnif = m_context->createUniformBuffer(172);
					m_instanceArgUnif = m_context->createUniformBuffer(64);

					m_argCommon->bindUniformBuffer(m_matGlobal, m_commonArgUnif);
					m_argInstance->bindUniformBuffer(m_matLocal, m_instanceArgUnif);
					//
				}
				{ // renderable
					std::vector<Nix::CVertex> vertices;
					std::vector<uint16_t> indices;
					m_renderable = m_material->createRenderable();
					CreateSphere(9, 16, vertices, indices);

					m_vertexBuffer = m_context->createVertexBuffer(vertices.data(), vertices.size() * sizeof(CVertex));
					m_indexBuffer = m_context->createIndexBuffer(indices.data(), sizeof(uint16_t) * indices.size());
					m_indexCount = indices.size();
					m_renderable->setIndexBuffer( m_indexBuffer, 0);
					m_renderable->setVertexBuffer(m_vertexBuffer, 0, 0);
				}
			}

			Config config;
			Nix::TextReader cfgReader;
			cfgReader.openFile(_archieve, "config/common.json");
			config.parse(cfgReader.getText());

			m_view = glm::lookAt(
				glm::vec3( config.eye[0], config.eye[1], config.eye[2] ), 
				glm::vec3( config.lookat[0], config.lookat[1], config.lookat[2]),
				glm::vec3( config.up[0], config.up[1], config.up[2])
				);
			m_light = glm::vec3( config.light[0], config.light[1], config.light[2] );

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
			//
			m_mainRenderPass->setViewport(vp);
			m_mainRenderPass->setScissor(ss);
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
					m_argCommon->updateUniformBuffer( m_commonArgUnif, &m_projection, 0, 64);
					m_argCommon->updateUniformBuffer(m_commonArgUnif, &m_view, 64, 64);
					m_argCommon->updateUniformBuffer(m_commonArgUnif, &m_light, 128, sizeof(m_light));
					m_argInstance->updateUniformBuffer( m_instanceArgUnif, &m_model, 0, 64);
					//
					m_mainRenderPass->bindPipeline(m_pipeline);
					m_mainRenderPass->bindArgument(m_argCommon);
					m_mainRenderPass->bindArgument(m_argInstance);
					//					
					m_mainRenderPass->drawElements( m_renderable, 0, m_indexCount);
				}
				m_mainRenderPass->end();

				m_context->endFrame();
			}			
		}

		virtual const char * title() {
			return "Sphere";
		}

		virtual uint32_t rendererType() {
			return 0;
		}
	};
}

Nix::Sphere theapp;

NixApplication* GetApplication() {
    return &theapp;
}