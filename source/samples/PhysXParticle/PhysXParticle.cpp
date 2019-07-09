#include "PhysXParticle.h"
#include "PhysXSystem.h"
#include "PhysXScene.h"
#include "ParticleManager.h"

#define STB_IMAGE_IMPLEMENTATION
#ifdef _WIN32
    #include <Windows.h>
#endif

namespace Nix {

	static physx::PxDefaultErrorCallback gDefaultErrorCallback;
	static physx::PxDefaultAllocator gDefaultAllocatorCallback;

	static ParticleEmiter emiter( physx::PxVec3(0, 2, 0), 3.1415926f/6.0f, 2.0f );

	const float perspectiveNear = 0.1f;
	const float perspectiveFar = 20.0f;
	const float perspectiveFOV = 3.1415926f / 2;
	const float particleSize = 0.4;

	float PT_VERTICES[20 * 20 * 3] = {};

	void PhysXParticle::onMouseEvent(eMouseButton _bt, eMouseEvent _event, int _x, int _y)
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

	void PhysXParticle::onKeyEvent(unsigned char _key, eKeyEvent _event)
	{
		if (_event == NixApplication::eKeyDown)
		{
			switch (_key)
			{
			case 'W':
			case 'w':
				m_camera.Forward(0.1f);
				break;
			case 'a':
			case 'A':
				m_camera.Leftward(0.1f);
				break;
			case 's':
			case 'S':
				m_camera.Backward(0.1f);
				break;
			case 'd':
			case 'D':
				m_camera.Rightward(0.1f);
				break;
			}
		}
	}


	bool PhysXParticle::initialize(void* _wnd, Nix::IArchieve* _archieve)
	{
		printf("%s", "PhysXParticle is initializing!");

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
		mtlReader.openFile(_archieve, "material/particle.json");
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
		texDesc.mipmapLevel = 4;
		texDesc.type = Nix::TextureType::Texture2D;

		bool rst = false;
		m_material = m_context->createMaterial(mtlDesc); {
			{ // graphics pipeline 
				m_pipeline = m_material->createPipeline(rpDesc);
			}
			{ // arguments
				m_argCommon = m_material->createArgument(0);
				rst = m_argCommon->getUniformBlock("Argument", &m_argSlot);
			}
			{ // renderable
				for (uint32_t row = 0; row < 20; ++row) {
					for (uint32_t col = 0; col < 20; ++col) {
						PT_VERTICES[(row * 20 + col) * 3] = row;
						PT_VERTICES[(row * 20 + col) * 3 + 1] = 0.5f;
						PT_VERTICES[(row * 20 + col) * 3 + 2] = col;
					}
				}
				m_renderable = m_material->createRenderable();
				//m_vertexBuffer = m_context->createStaticVertexBuffer(PT_VERTICES, sizeof(PT_VERTICES));
				m_vertexBuffer = m_context->createStaticVertexBuffer(nullptr, sizeof(glm::vec3) * 4098);
				m_renderable->setVertexBuffer(m_vertexBuffer, 0, 0);
			}
		}
		m_camera.SetLookAt(glm::vec3(0, 0, 0));
		m_camera.SetEye(glm::vec3(-2, 2, -2));

		// initialize physx
		m_timePoint = std::chrono::system_clock::now();
		m_phySystem = new PhysXSystem();
		m_phySystem->initialize();
		m_phyScene = m_phySystem->createScene();


		return true;
	}

	void PhysXParticle::resize(uint32_t _width, uint32_t _height)
	{
		printf("resized!");
		m_context->resize(_width, _height);
		Viewport vp = {
			0.0f, 0.0f, (float)_width, (float)_height, 0.0f, 1.0f
		};
		Scissor ss;
		ss.origin = { 0, 0 };
		ss.size = { (int)_width, (int)_height };
		m_camera.Perspective(perspectiveFOV, (float)_width / (float)_height, perspectiveNear, perspectiveFar);
		m_pipeline->setViewport(vp);
		m_pipeline->setScissor(ss);
		wndWidth = _width;
		wndHeight = _height;
	}

	void PhysXParticle::release()
	{
		printf("destroyed");
		m_context->release();
	}

	void PhysXParticle::tick()
	{
		auto now = std::chrono::system_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now - m_timePoint); 
		float dt = double(duration.count()) * std::chrono::microseconds::period::num / std::chrono::microseconds::period::den;
		//dt = dt / 1000.0f;
		m_timePoint = now;
		m_phyScene->simulate(dt);
		static std::vector<physx::PxVec3> vertices;
		vertices.clear();
		m_phyScene->getParticlePrimitivePositions(vertices);
		uint32_t pointCount = 0;
		if ( vertices.size() < 4096) {
			pointCount = vertices.size();
		}
		else {
			pointCount = 4096;
		}
		static uint64_t frameCounter = 0;
		++frameCounter;
		if (frameCounter % 16 == 0) {
			m_phyScene->addParticlePrimitive(PxVec3(0, 2, 0), emiter.emit());
		}
		

		m_camera.Tick();
		static uint64_t tickCounter = 0;
		//m_phyScene

		tickCounter++;

		float imageIndex = (tickCounter / 1024) % 4;

		if (m_context->beginFrame()) {

			if (vertices.size()) {
				m_vertexBuffer->setData(vertices.data(), pointCount * sizeof(physx::PxVec3), 0);
			}		

			m_mainRenderPass->begin(m_primQueue); {
				glm::mat4x4 identity;
				m_argCommon->setUniform(m_argSlot, 0, &m_camera.GetViewMatrix(), 64);
				m_argCommon->setUniform(m_argSlot, 64, &m_camera.GetProjMatrix(), 64);
				m_argCommon->setUniform(m_argSlot, 128, &wndWidth, 4);
				m_argCommon->setUniform(m_argSlot, 132, &wndHeight, 4);
				m_argCommon->setUniform(m_argSlot, 136, &perspectiveNear, 4);
				m_argCommon->setUniform(m_argSlot, 140, &perspectiveFOV, 4);
				m_argCommon->setUniform(m_argSlot, 144, &particleSize, 4);

				//
				m_mainRenderPass->bindPipeline(m_pipeline);
				m_mainRenderPass->bindArgument(m_argCommon);
				//
				//m_mainRenderPass->drawElements(m_renderable, 0, 6);
				m_mainRenderPass->draw(m_renderable, 0, pointCount);
			}
			m_mainRenderPass->end();
			//
			m_context->endFrame();
		}
	}

	const char * PhysXParticle::title()
	{
		return "PhysXParticle";
	}

	uint32_t PhysXParticle::rendererType()
	{
		return 0;
	}

}

Nix::PhysXParticle theapp;

NixApplication* GetApplication() {
    return &theapp;
}