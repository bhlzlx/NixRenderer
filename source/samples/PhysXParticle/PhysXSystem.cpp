#include "PhysXSystem.h"
#include "PhysXScene.h"

namespace Nix {

	static PxDefaultErrorCallback gDefaultErrorCallback;
	static PxDefaultAllocator gDefaultAllocatorCallback;

// 	PxFilterFlags NixSimulationFilterShader(
// 		PxFilterObjectAttributes attributes0,
// 		PxFilterData filterData0,
// 		PxFilterObjectAttributes attributes1,
// 		PxFilterData filterData1,
// 		PxPairFlags& pairFlags,
// 		const void* constantBlock,
// 		PxU32 constantBlockSize) {
// 
// 
// 
// 	}


	PhysXSystem::PhysXSystem(){

	}

	PhysXSystem::~PhysXSystem(){

	}

	bool PhysXSystem::initialize(){
		m_errorCallback = &gDefaultErrorCallback;
		m_allocCallback = &gDefaultAllocatorCallback;
		//
		m_foundation = PxCreateFoundation(PX_PHYSICS_VERSION, *m_allocCallback, *m_errorCallback);
		if (!m_foundation) {
			return false;
		}
		// connect to physics visual debugger!
		m_pvd = PxCreatePvd(*m_foundation);
		if (!m_pvd) {
			return false;
		}
		m_transport = PxDefaultPvdSocketTransportCreate("127.0.0.1", 2333, 50);
		m_pvd->connect(*m_transport, PxPvdInstrumentationFlag::eALL);
		// create physics object
		m_scale = PxTolerancesScale();
		m_physics = PxCreatePhysics(PX_PHYSICS_VERSION, *m_foundation, m_scale, false, m_pvd);
		if (!m_physics) {
			return false;
		}
		PxCookingParams cookingParams(m_scale);
		cookingParams.meshPreprocessParams = PxMeshPreprocessingFlags(PxMeshPreprocessingFlag::eDISABLE_CLEAN_MESH);
		cookingParams.buildGPUData = false; //Enable GRB data being produced in cooking.
		m_cooking = PxCreateCooking(PX_PHYSICS_VERSION, *m_foundation, cookingParams);
		if (!m_cooking) {
			return false;
		}
		m_material = m_physics->createMaterial(0.5f, 0.5f, 0.1f);
		// cpu dispatcher with 2 thread
		m_cpuDispatcher = PxDefaultCpuDispatcherCreate(2);
		//
		return true;
	}

	void PhysXSystem::shutdown(){
		delete m_cpuDispatcher;
		m_cpuDispatcher = nullptr;
	}

	Nix::PhysXScene* PhysXSystem::createScene()
	{
		PxSceneDesc sceneDesc(m_scale);
		sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
		sceneDesc.cpuDispatcher = m_cpuDispatcher;
		sceneDesc.filterShader = PxDefaultSimulationFilterShader;
		PhysXScene* scene = new PhysXScene();
		bool rst = scene->initialize(this, sceneDesc);
		if (!rst) {
			delete scene;
			return nullptr;
		}
		return scene;
	}

}