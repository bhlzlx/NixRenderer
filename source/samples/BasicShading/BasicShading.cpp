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

#include "../FreeCamera.h"
#include "BasicShading.h"
#include "BuddySystemAllocator.h"
#include <stack>

#ifdef _WIN32
    #include <Windows.h>
#endif

#define camStep 0.01

namespace Nix {

	std::vector<CVertex> modelVertices;

	bool Sphere::initialize(void* _wnd, Nix::IArchieve* _archieve)
	{
		printf("%s", "Sphere is initializing!");

		HMODULE library = ::LoadLibraryA("NixVulkan.dll");
		assert(library);

		typedef IDriver*(*PFN_CREATE_DRIVER)();

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
		mtlReader.openFile(_archieve, "material/basicRendering.json");
		mtlDesc.parse(mtlReader.getText());
		RenderPassDescription rpDesc;
		Nix::TextReader rpReader;
		rpReader.openFile(_archieve, "renderpass/swapchain.json");
		rpDesc.parse(rpReader.getText());
		rpDesc.colors[0].format = m_context->swapchainColorFormat();
		rpDesc.depthStencil.format = m_driver->selectDepthFormat(true);

		Nix::IFile * texFile = _archieve->open("texture/normalMap.ktx");
		Nix::IFile* texMem = CreateMemoryBuffer(texFile->size());
		texFile->read(texFile->size(), texMem);
		m_normalMap = m_context->createTextureKTX(texMem->constData(), texMem->size());


		Nix::TextReader objTextReader;
		bool readRst = objTextReader.openFile(m_arch, "model/Boxset/box_stack.obj");
		assert(readRst);
		Nix::TextReader mtlTextReader;
		readRst = mtlTextReader.openFile(m_arch, "model/Boxset/box_stack.mtl");
		assert(readRst);
		tinyobj::ObjReader objReader;
		tinyobj::ObjReaderConfig objReaderConfig;
		objReader.ParseFromString(objTextReader.getText(), mtlTextReader.getText(), objReaderConfig);

		for (size_t shapeIndex = 0; shapeIndex < objReader.GetShapes().size(); ++shapeIndex) {
			auto& indices = objReader.GetShapes().at(shapeIndex).mesh.indices;
			for (auto index : indices) {
				CVertex vertex;
				vertex.xyz.x = objReader.GetAttrib().vertices[index.vertex_index * 3];
				vertex.xyz.y = objReader.GetAttrib().vertices[index.vertex_index * 3 + 1];
				vertex.xyz.z = objReader.GetAttrib().vertices[index.vertex_index * 3 + 2];
				vertex.coord.x = objReader.GetAttrib().texcoords[index.texcoord_index * 2];
				vertex.coord.y = objReader.GetAttrib().texcoords[index.texcoord_index * 2 + 1];
				vertex.norm.x = objReader.GetAttrib().normals[index.normal_index * 3];
				vertex.norm.y = objReader.GetAttrib().normals[index.normal_index * 3 + 1];
				vertex.norm.z = objReader.GetAttrib().normals[index.normal_index * 3 + 2];
				modelVertices.push_back(vertex);
			}
		}

		for (size_t vertexIndex = 0; vertexIndex < modelVertices.size() / 3; ++vertexIndex) {
			glm::mat3x2 TB = calcTB(
				modelVertices[vertexIndex * 3 + 1].xyz - modelVertices[vertexIndex * 3].xyz,
				modelVertices[vertexIndex * 3 + 2].xyz - modelVertices[vertexIndex * 3].xyz,
				modelVertices[vertexIndex * 3 + 1].coord - modelVertices[vertexIndex * 3].coord,
				modelVertices[vertexIndex * 3 + 2].coord - modelVertices[vertexIndex * 3].coord
			);
			glm::vec3 tangent = glm::vec3(TB[0].x, TB[1].x, TB[2].x);
			glm::vec3 bitangent = glm::vec3(TB[0].y, TB[1].y, TB[2].y);
			modelVertices[vertexIndex * 3 + 1].tangent = modelVertices[vertexIndex * 3 + 2].tangent = modelVertices[vertexIndex * 3].tangent = tangent;
			modelVertices[vertexIndex * 3 + 1].bitangent = modelVertices[vertexIndex * 3 + 2].bitangent = modelVertices[vertexIndex * 3].bitangent = bitangent;
		}

		BuddySystemAllocator bufferAllocator;
	
		IBuffer* buffer = m_context->createStaticVertexBuffer(nullptr, 4096*1024 );// 4MB
		bufferAllocator.initialize(4096 * 1024, 128 * 1024);

		std::stack<size_t> offsets;
		std::stack<uint16_t> ids;
		size_t offset;
		uint16_t id;

		bool rst = false;
		rst = bufferAllocator.allocate(179, offset, id); offsets.push(offset); ids.push(id);
		rst = bufferAllocator.allocate(35, offset, id); offsets.push(offset); ids.push(id);
		rst = bufferAllocator.allocate(18, offset, id); offsets.push(offset); ids.push(id);
		rst = bufferAllocator.allocate(224, offset, id); offsets.push(offset); ids.push(id);

		bufferAllocator.free(ids.top()); offsets.pop(); ids.pop();
		bufferAllocator.free(ids.top()); offsets.pop(); ids.pop();
		bufferAllocator.free(ids.top()); offsets.pop(); ids.pop();
		bufferAllocator.free(ids.top()); offsets.pop(); ids.pop();


		rst = bufferAllocator.allocate(18, offset, id); offsets.push(offset); ids.push(id);
		rst = bufferAllocator.allocate(35, offset, id); offsets.push(offset); ids.push(id);
		rst = bufferAllocator.allocate(179, offset, id); offsets.push(offset); ids.push(id);
		rst = bufferAllocator.allocate(224, offset, id); offsets.push(offset); ids.push(id);

	

		TextureDescription texDesc;
		texDesc.depth = 1;
		texDesc.format = NixRGBA8888_UNORM;
		texDesc.height = 64;
		texDesc.width = 64;
		texDesc.mipmapLevel = 1;
		texDesc.type = Nix::TextureType::Texture2D;

		rst = false;
		m_material = m_context->createMaterial(mtlDesc); {
			{ // graphics pipeline 
				m_pipeline = m_material->createPipeline(rpDesc);
			}
			{ // arguments
				m_argCommon = m_material->createArgument(0);
				rst = m_argCommon->getUniformBlock("GlobalArgument", &m_matGlobal);
				uint32_t normalMapSlot;
				rst = m_argCommon->getSampler("normalMap", &normalMapSlot);
				SamplerState ss;
				m_argCommon->setSampler(normalMapSlot, ss, m_normalMap);
				m_argInstance = m_material->createArgument(1);
				rst = m_argInstance->getUniformBlock("LocalArgument", &m_matLocal);
			}
			{ // renderable
				//std::vector<Nix::CVertex> vertices;
				//std::vector<uint16_t> indices;
				m_renderable = m_material->createRenderable();
				m_vertexBuffer = m_context->createStaticVertexBuffer(modelVertices.data(), modelVertices.size() * sizeof(CVertex));
				m_renderable->setVertexBuffer(m_vertexBuffer, 0, 0);
			}
		}

		Config config;
		Nix::TextReader cfgReader;
		cfgReader.openFile(_archieve, "config/common.json");
		config.parse(cfgReader.getText());

		m_camera.SetEye(glm::vec3(config.eye[0], config.eye[1], config.eye[2]));
		m_camera.SetLookAt(glm::vec3(config.lookat[0], config.lookat[1], config.lookat[2]));
		m_camera.SetTop(glm::vec3(config.up[0], config.up[1], config.up[2]));

		m_light = glm::vec3(config.light[0], config.light[1], config.light[2]);
		return true;
	}

	void Sphere::resize(uint32_t _width, uint32_t _height)
	{
		printf("resized!");
		m_context->resize(_width, _height);
		Viewport vp = {
			0.0f, 0.0f, (float)_width, (float)_height, 0.0f, 1.0f
		};
		Scissor ss;
		ss.origin = { 0, 0 };
		ss.size = { (int)_width, (int)_height };
		//
		m_camera.Perspective(45.0f / 180.0f * 3.1415926f, (float)_width / (float)_height, 0.01f, 200.0f);
		//
		m_pipeline->setViewport(vp);
		m_pipeline->setScissor(ss);
	}

	void Sphere::release()
	{
		printf("destroyed");
		m_context->release();
	}

	void Sphere::tick()
	{
		m_camera.Tick();
		static float angle = 0.0f;
		angle += .5f;
		m_model = glm::rotate<float>(glm::mat4(), angle / 180.0f * 3.1415926, glm::vec3(0, 1, 0));
		if (m_context->beginFrame()) {
			m_mainRenderPass->begin(m_primQueue); {
				glm::mat4x4 identity;
				m_argCommon->setUniform(m_matGlobal, 0, &m_camera.GetProjMatrix(), 64);
				m_argCommon->setUniform(m_matGlobal, 64, &m_camera.GetViewMatrix(), 64);
				m_argCommon->setUniform(m_matGlobal, 128, &m_light, sizeof(m_light));
				m_argCommon->setUniform(m_matGlobal, 144, &m_camera.GetEye(), sizeof(glm::vec3));
				//
				m_argInstance->setUniform(m_matLocal, 0, &m_model, 64);
				//
				m_mainRenderPass->bindPipeline(m_pipeline);
				m_mainRenderPass->bindArgument(m_argCommon);
				m_mainRenderPass->bindArgument(m_argInstance);
				//					
				//m_mainRenderPass->drawElements( m_renderable, 0, m_indexCount);
				m_mainRenderPass->draw(m_renderable, 0, modelVertices.size());
			}
			m_mainRenderPass->end();

			m_context->endFrame();
		}
	}

	const char * Sphere::title()
	{
		return "Sphere";
	}

	uint32_t Sphere::rendererType()
	{
		return 0;
	}

	void Sphere::onKeyEvent(unsigned char _key, eKeyEvent _event)
	{
		if (_event == NixApplication::eKeyDown)
		{
			switch (_key)
			{
			case 'W':
			case 'w':
				m_camera.Forward(camStep);
				break;
			case 'a':
			case 'A':
				m_camera.Leftward(camStep);
				break;
			case 's':
			case 'S':
				m_camera.Backward(camStep);
				break;
			case 'd':
			case 'D':
				m_camera.Rightward(camStep);
				break;
			}
		}
	}

	void Sphere::onMouseEvent(eMouseButton _bt, eMouseEvent _event, int _x, int _y)
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

}

Nix::Sphere theapp;

NixApplication* GetApplication() {
    return &theapp;
}