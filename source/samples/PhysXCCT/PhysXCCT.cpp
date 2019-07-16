#include "PhysXCCT.h"
#ifdef _WIN32
    #include <Windows.h>
#endif
#include "nix\string\path.h"

namespace Nix {

	void PhysXCCT::onMouseEvent(eMouseButton _bt, eMouseEvent _event, int _x, int _y)
	{
	}

	void PhysXCCT::onKeyEvent(unsigned char _key, eKeyEvent _event)
	{
	}


	bool PhysXCCT::initialize(void* _wnd, Nix::IArchieve* _archieve)
	{
		static PxDefaultAllocator allocator;
		static NixPhysicsErrorCallback errorCallback;
		//
		m_pxFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, allocator, errorCallback);
		m_pxPVD = PxCreatePvd(*m_pxFoundation);
		m_transport = PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 50);
		m_pxPVD->connect(*m_transport, PxPvdInstrumentationFlag::eALL);
		//
		m_scale = PxTolerancesScale();
		m_pxSDK = PxCreatePhysics(PX_PHYSICS_VERSION, *m_pxFoundation, m_scale, false, m_pxPVD);
		//
		PxCookingParams cookingParams(m_scale);
		cookingParams.meshPreprocessParams = PxMeshPreprocessingFlags(PxMeshPreprocessingFlag::eDISABLE_CLEAN_MESH);
		cookingParams.buildGPUData = false; //Enable GRB data being produced in cooking.
		m_pxCooking = PxCreateCooking(PX_PHYSICS_VERSION, *m_pxFoundation, cookingParams);
		if (!m_pxCooking) {
			return false;
		}
		m_pxMaterial = m_pxSDK->createMaterial(0.5f, 0.5f, 0.1f);
		// cpu dispatcher with 2 thread
		m_cpuDispatcher = PxDefaultCpuDispatcherCreate(2);



		//  create scene
		PxSceneDesc sceneDesc(m_scale);
		sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
		sceneDesc.cpuDispatcher = m_cpuDispatcher;
		sceneDesc.filterShader = PxDefaultSimulationFilterShader;
		m_pxScene = m_pxSDK->createScene(sceneDesc);
		//m_scene->setContactModifyCallback()
		if (!m_pxScene) {
			return false;
		}
		m_pxScenePvdClient = m_pxScene->getScenePvdClient();
		m_pxScene->setSimulationEventCallback(&m_simulateEventCallback);
		if (m_pxScenePvdClient) {
			m_pxScenePvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
			m_pxScenePvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
			m_pxScenePvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
		}
		m_pxMaterial = m_pxSDK->createMaterial(0.5f, 0.5f, 0.6f);
		PxRigidStatic* groundPlane = PxCreatePlane(*m_pxSDK, PxPlane(0, 1, 0, 0), *m_pxMaterial);
		
		PxShape* shape = nullptr;
		groundPlane->getShapes(&shape, sizeof(PxShape*));
		assert(shape);
		PxFilterData filterData;

		shape->setFlag(PxShapeFlag::eSCENE_QUERY_SHAPE, true);
		shape->setFlag(PxShapeFlag::eVISUALIZATION, true);
		shape->setSimulationFilterData(filterData);
		m_pxScene->addActor(*groundPlane);
		//
		m_controllerManager = PxCreateControllerManager(*m_pxScene);
		PxCapsuleControllerDesc capsuleDesc; {
			capsuleDesc.climbingMode = PxCapsuleClimbingMode::eCONSTRAINED;
			capsuleDesc.height = 1.0f;
			capsuleDesc.radius = 0.4f;
			capsuleDesc.behaviorCallback = &m_controllerBehaviorCallback;
			capsuleDesc.reportCallback = &m_controllerHitReport;
			capsuleDesc.material = m_pxMaterial;
			capsuleDesc.nonWalkableMode = PxControllerNonWalkableMode::ePREVENT_CLIMBING;
			capsuleDesc.position = PxExtendedVec3(0.0f, 10.0f, 0.0f);
			capsuleDesc.userData = this;
			//
			capsuleDesc.contactOffset; // default by 0.1f
			capsuleDesc.density; // default by 10.0f
			capsuleDesc.invisibleWallHeight; // default by 0.0f
			capsuleDesc.maxJumpHeight = 1.0f;
		}
		m_controller = m_controllerManager->createController(capsuleDesc);
		//
		m_controllerFilters.mCCTFilterCallback = nullptr;
		m_controllerFilters.mFilterCallback = &m_controllerQueryFilterCallback;
		m_controllerFilters.mFilterData;
		m_controllerFilters.mFilterFlags = PxQueryFlag::eSTATIC | PxQueryFlag::ePREFILTER;
		//
		return true;
	}

	void PhysXCCT::resize(uint32_t _width, uint32_t _height)
	{
	}

	void PhysXCCT::release()
	{
		m_pxScene->fetchResults(true);
		//
		m_pxCooking->release();
		m_pxMaterial->release();
		m_cpuDispatcher->release();
		m_pxScene->release();
		m_pxSDK->release();
		m_pxPVD->release();
		m_transport->release();
		m_pxFoundation->release();
	}

	void PhysXCCT::tick()
	{
		auto now = std::chrono::system_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now - m_timePoint);
		float dt = double(duration.count()) * std::chrono::microseconds::period::num / std::chrono::microseconds::period::den;
		//dt = dt / 1000.0f;
		if (dt > 0.1f) {
			dt = 0.1f;
		}
		m_timePoint = now;
		m_pxScene->simulate(dt);
		m_pxScene->fetchResults(true);

		PxVec3 disp(0.1f, -0.2f, 0.1f);
		m_controller->move(disp, 0.01, dt, m_controllerFilters, nullptr);
	}

	const char * PhysXCCT::title()
	{
		return "PhysXCCT";
	}

	uint32_t PhysXCCT::rendererType()
	{
		return 0;
	}

	void NixPhysicsEventCallback::onConstraintBreak(PxConstraintInfo* constraints, PxU32 count) {
		return;
	}

	void NixPhysicsEventCallback::onWake(PxActor** actors, PxU32 count)	{
		return;
	}

	void NixPhysicsEventCallback::onSleep(PxActor** actors, PxU32 count)	{
		return;
	}

	void NixPhysicsEventCallback::onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs)	{
		return;
	}

	void NixPhysicsEventCallback::onTrigger(PxTriggerPair* pairs, PxU32 count){
		return;
	}

	void NixPhysicsEventCallback::onAdvance(const PxRigidBody*const* bodyBuffer, const PxTransform* poseBuffer, const PxU32 count){
		return;
	}

	physx::PxControllerBehaviorFlags NixControllerBehaviorCallback::getBehaviorFlags(const PxShape& shape, const PxActor& actor)
	{
		return PxControllerBehaviorFlag::eCCT_CAN_RIDE_ON_OBJECT;
	}

	physx::PxControllerBehaviorFlags NixControllerBehaviorCallback::getBehaviorFlags(const PxController& controller)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	physx::PxControllerBehaviorFlags NixControllerBehaviorCallback::getBehaviorFlags(const PxObstacle& obstacle)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void NixUserControllerHitReport::onShapeHit(const PxControllerShapeHit& hit)
	{
		return;
	}

	void NixUserControllerHitReport::onControllerHit(const PxControllersHit& hit)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void NixUserControllerHitReport::onObstacleHit(const PxControllerObstacleHit& hit)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	physx::PxQueryHitType::Enum NixControllerQueryFilterCallback::preFilter(const PxFilterData& filterData, const PxShape* shape, const PxRigidActor* actor, PxHitFlags& queryFlags)
	{
		return PxQueryHitType::eBLOCK;
	}

	physx::PxQueryHitType::Enum NixControllerQueryFilterCallback::postFilter(const PxFilterData& filterData, const PxQueryHit& hit)
	{
		return PxQueryHitType::eBLOCK;
	}

}

Nix::PhysXCCT theapp;

NixApplication* GetApplication() {
    return &theapp;
}