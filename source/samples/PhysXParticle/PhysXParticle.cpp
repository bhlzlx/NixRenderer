#include "PhysXParticle.h"
#include "PhysXSystem.h"
#include "PhysXScene.h"
#include "ParticleManager.h"
#include "PhysXControllerManager.h"
#include "Player.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#ifdef _WIN32
    #include <Windows.h>
#endif
#include "nix\string\path.h"

namespace Nix {

	PxVec3 moveDirection;

	PhysxControllerManager* controllerManager = nullptr;
	Player* player = nullptr;

	static physx::PxDefaultErrorCallback gDefaultErrorCallback;
	static physx::PxDefaultAllocator gDefaultAllocatorCallback;

	static ParticleEmiter emiter( physx::PxVec3(0, 4, 0), 3.1415926f/9.0f, 5.0f );

	// 写在前边
	// 我们规定高度图每两个像素之间是长度为1的单位
	// 同样高度图最大值为1.0f, 所以使用 stb_image读出来的uint8_t最大值255应该除255.0f

	const float perspectiveNear = 0.1f;
	const float perspectiveFar = 512.0f;
	const float perspectiveFOV = 3.1415926f / 2;
	const float particleSize = 2.0f;

	const float heightFieldXScale = 1.0f;
	const float heightFieldYScale = 8.0f;
	const float heightFieldZScale = 1.0f;

	const float heightFieldOffsetX =  -32.0f;
	const float heightFieldOffsetY = 0.0f;
	const float heightFieldOffsetZ =  -32.0f;

	const float cammeraStepDistance = 0.5f;

	PxVec3 emitSource;
	PxVec3 emitSource1;

	uint32_t hfWidth = 0;
	uint32_t hfHeight = 0;

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
				moveDirection = PxVec3(m_camera.GetForword().x, m_camera.GetForword().y, m_camera.GetForword().z);
				//m_camera.Forward(cammeraStepDistance);
				break;
			case 'a':
			case 'A':
				m_camera.Leftward(cammeraStepDistance);
				break;
			case 's':
			case 'S':
				m_camera.Backward(cammeraStepDistance);
				break;
			case 'd':
			case 'D':
				m_camera.Rightward(cammeraStepDistance);
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

		auto file = _archieve->open("texture/lightball.ktx");
		auto mem = CreateMemoryBuffer(file->size());
		file->read(file->size(), mem);
		m_texture = m_context->createTextureKTX(mem->constData(), mem->size());
		mem->release();
		file->release();


		bool rst = false;
		m_material = m_context->createMaterial(mtlDesc); {
			{ // graphics pipeline 
				m_pipeline = m_material->createPipeline(rpDesc);
			}
			{ // arguments
				m_argCommon = m_material->createArgument(0);
				rst = m_argCommon->getUniformBlock("Argument", &m_argSlot);
				SamplerState ss;
				ss.min = TexFilterPoint;
				ss.mag = TexFilterPoint;
				ss.u = AddressModeClamp;
				ss.v = AddressModeClamp;
				m_argCommon->setSampler(0, ss, m_texture);
			}
			{ // renderable
				m_renderable = m_material->createRenderable();
				//m_vertexBuffer = m_context->createStaticVertexBuffer(PT_VERTICES, sizeof(PT_VERTICES));
				m_vertexBuffer = m_context->createStaticVertexBuffer(nullptr, sizeof(glm::vec3) * 4098);
				m_renderable->setVertexBuffer(m_vertexBuffer, 0, 0);
			}
		}

		// initialize physx
		m_timePoint = std::chrono::system_clock::now();
		m_phySystem = new PhysXSystem();
		m_phySystem->initialize();
		m_phyScene = m_phySystem->createScene();

		Nix::TextReader mtlHFReader;
		mtlHFReader.openFile(_archieve, "material/heightField.json");
		Nix::MaterialDescription mtlHFDesc;
		mtlHFDesc.parse(mtlHFReader.getText());

		m_mtlHF = m_context->createMaterial(mtlHFDesc);
		m_pipelineHF = m_mtlHF->createPipeline(rpDesc);
		m_argHF = m_mtlHF->createArgument(0);


		//std::string filepath = std::string(_archieve->root()) + "/texture/heightmap.png";
		std::string filepath = std::string(_archieve->root()) + "/texture/terrian.png";
		filepath = FormatFilePath(filepath);
		int width, height, channel;
		stbi_uc * bytes = stbi_load(filepath.c_str(), &width, &height, &channel, 1);
		m_phyScene->addHeightField(
			(uint8_t*)bytes,
			width, height,
			PxVec3(heightFieldOffsetX, heightFieldOffsetY, heightFieldOffsetZ),
			PxVec3(heightFieldXScale, heightFieldYScale, heightFieldZScale)
		);

		hfWidth = width;
		hfHeight = height;

		// 因为我们刚刚添加了高度图，地形信息，所以我们可以立即查询一个相交点
		// 随机两个粒子发射位置
		// 随机一个镜头初始位置
		static std::default_random_engine random(time(NULL));
		std::uniform_int_distribution<int> dis1( -(int)hfWidth/4, hfWidth/4 );
		int randX = dis1(random) * heightFieldXScale;
		int randZ = dis1(random) * heightFieldZScale;
		bool raycastRst = m_phyScene->raycast(PxVec3(randX, 40, randZ), PxVec3(0, -1, 0), 100, emitSource);
		assert(raycastRst);
		emitSource.y = emitSource.y + 1;

		randX = dis1(random) * heightFieldXScale;
		randZ = dis1(random) * heightFieldZScale;
		raycastRst = m_phyScene->raycast(PxVec3(randX, 40, randZ), PxVec3(0, -1, 0), 100, emitSource1);
		assert(raycastRst);
		emitSource1.y = emitSource1.y + 1;

		randX = dis1(random) * heightFieldXScale;
		randZ = dis1(random) * heightFieldZScale;
		PxVec3 eye;
		raycastRst = m_phyScene->raycast(PxVec3(randX, 40, randZ), PxVec3(0, -1, 0), 100, eye);
		assert(raycastRst);
		eye.y = eye.y + 4;

		m_camera.SetLookAt(glm::vec3(0, 0, 0));
		m_camera.SetEye(glm::vec3( eye.x, eye.y, eye.z ));

		controllerManager = m_phyScene->createControllerManager();
		PxVec3 controllerPos;
		randX = dis1(random) * heightFieldXScale;
		randZ = dis1(random) * heightFieldZScale;
		raycastRst = m_phyScene->raycast(PxVec3(randX, 40, randZ), PxVec3(0, -1, 0), 100, controllerPos);
		assert(raycastRst);
		controllerPos.y += 4.0f;

		player = new Player();
		player->initialize(controllerManager, controllerPos);

		TextureDescription hfTexDesc;
		hfTexDesc.depth = 1;
		hfTexDesc.format = NixFormat::NixR8_UNORM;
		hfTexDesc.width = width;
		hfTexDesc.height = height;
		hfTexDesc.mipmapLevel = 1;
		hfTexDesc.type = Texture2D;
		m_texHF = m_context->createTexture(hfTexDesc);
		BufferImageUpload ud;
		ud.baseMipRegion.baseLayer = 0;
		ud.baseMipRegion.mipLevel = 0;
		ud.baseMipRegion.offset.x = ud.baseMipRegion.offset.y = ud.baseMipRegion.offset.z = 0;
		ud.length = sizeof(uint8_t) * width * height;
		ud.data = bytes;
		ud.mipDataOffsets[0] = 0;
		ud.mipCount = 1;
		ud.baseMipRegion.size.width = width;
		ud.baseMipRegion.size.height = height;
		ud.baseMipRegion.size.depth = 1;

		m_texHF->uploadSubData(ud);
		std::vector<uint16_t> indicesDataHF;// .reserve((width - 1) * 6 * height);
		for (uint32_t r = 0; r < height - 1; ++r) {
			for (uint32_t c = 0; c<width-1; ++c) {
				uint16_t upleft = r * width + c;
				uint16_t downleft = upleft + width;
				uint16_t upright = upleft + 1;
				uint16_t downright = downleft + 1;
				indicesDataHF.push_back(upleft); indicesDataHF.push_back(downleft); indicesDataHF.push_back(downright);
				indicesDataHF.push_back(upleft); indicesDataHF.push_back(downright); indicesDataHF.push_back(upright);
			}
		}
		m_indicesHF = m_context->createIndexBuffer(indicesDataHF.data(), indicesDataHF.size() * 2);


		SamplerState ss;
		ss.min = TexFilterLinear;
		ss.mag = TexFilterLinear;
		m_argHF->setSampler(0, ss, m_texHF);
		m_renderableHF = m_mtlHF->createRenderable();
		m_renderableHF->setIndexBuffer(m_indicesHF, 0);

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
		m_pipelineHF->setViewport(vp);
		m_pipeline->setScissor(ss);
		m_pipelineHF->setScissor(ss);
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
		if (dt > 0.1f) {
			dt = 0.1f;
		}
		m_timePoint = now;
		m_phyScene->simulate(dt);
		static std::vector<physx::PxVec3> vertices;
		vertices.clear();
		m_phyScene->getParticlePrimitivePositions(vertices);
		if (!moveDirection.isZero()) {
			player->move(moveDirection, dt);
			moveDirection.x = moveDirection.y = moveDirection.z = 0;
		}
		player->tick(dt);
		auto playerPos = player->getPosition();
		m_camera.SetEye( glm::vec3(playerPos.x, playerPos.y, playerPos.z) );

		uint32_t pointCount = 0;
		if ( vertices.size() < 4096) {
			pointCount = vertices.size();
		}
		else {
			pointCount = 4096;
		}
		static uint64_t frameCounter = 0;
		++frameCounter;

		if (frameCounter % 4 == 0) {
			m_phyScene->addParticlePrimitive(emitSource, emiter.emit());
			m_phyScene->addParticlePrimitive(emitSource1, emiter.emit());
		}
		

		m_camera.Tick();
		static uint64_t tickCounter = 0;
		//m_phyScene

		tickCounter++; 

		if (m_context->beginFrame()) {

			if (vertices.size()) {
				m_vertexBuffer->setData(vertices.data(), pointCount * sizeof(physx::PxVec3), 0);
			}		

			m_mainRenderPass->begin(m_primQueue); {
				glm::mat4x4 identity;

				glm::mat4x4 viewMat = m_camera.GetViewMatrix();
				glm::mat4x4 projMat = m_camera.GetProjMatrix();
				glm::mat4x4 moveMat = glm::translate(identity, glm::vec3(heightFieldOffsetX, heightFieldOffsetY, heightFieldOffsetZ));
				glm::mat4x4 scaleMat = glm::scale(identity, glm::vec3(heightFieldXScale, heightFieldYScale, heightFieldZScale));
				glm::mat4x4 mvp = projMat * viewMat * moveMat * scaleMat;
				//
				m_argHF->setUniform(0, 0, &mvp, sizeof(mvp));
				m_argHF->setUniform(0, 64, &hfWidth, 4);
				m_argHF->setUniform(0, 68, &hfHeight, 4);

				m_mainRenderPass->bindPipeline(m_pipelineHF);
				m_mainRenderPass->bindArgument(m_argHF);
				m_mainRenderPass->drawElements(m_renderableHF, 0, (hfWidth - 1) * (hfHeight-1) * 6);

				// draw particles
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