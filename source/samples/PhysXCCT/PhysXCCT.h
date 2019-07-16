#pragma once
#include <NixApplication.h>
#include <nix/io/archieve.h>
#include <cstdio>
#include <cassert>
#include <PxPhysicsAPI.h>
#include <pvd/PxPvd.h>
#include <chrono>

// this demo does not rendering any thing!
// only connect to `PhysX Visual Debugger` for test

namespace Nix {

	using namespace physx;

	class NixPhysicsErrorCallback : public PxErrorCallback
	{
	public:
		NixPhysicsErrorCallback() {}
		~NixPhysicsErrorCallback() {}

		virtual void reportError(PxErrorCode::Enum code, const char* message, const char* file, int line) {
			::OutputDebugStringA(message);
		}
	};

	class NixPhysicsEventCallback : public PxSimulationEventCallback {

	public:
		virtual void onConstraintBreak(PxConstraintInfo* constraints, PxU32 count) override;
		virtual void onWake(PxActor** actors, PxU32 count) override;
		virtual void onSleep(PxActor** actors, PxU32 count) override;
		virtual void onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs) override;
		virtual void onTrigger(PxTriggerPair* pairs, PxU32 count) override;
		virtual void onAdvance(const PxRigidBody*const* bodyBuffer, const PxTransform* poseBuffer, const PxU32 count) override;
	};

	class NixControllerBehaviorCallback 
		: public PxControllerBehaviorCallback 
	{

	public:
		virtual PxControllerBehaviorFlags getBehaviorFlags(const PxShape& shape, const PxActor& actor) override;
		virtual PxControllerBehaviorFlags getBehaviorFlags(const PxController& controller) override;
		virtual PxControllerBehaviorFlags getBehaviorFlags(const PxObstacle& obstacle) override;
	};

	class NixUserControllerHitReport 
		: public PxUserControllerHitReport {
	public:
		virtual void onShapeHit(const PxControllerShapeHit& hit) override;
		virtual void onControllerHit(const PxControllersHit& hit) override;
		virtual void onObstacleHit(const PxControllerObstacleHit& hit) override;
	};

	class NixControllerQueryFilterCallback 
		: public PxQueryFilterCallback {
	public:
		virtual PxQueryHitType::Enum preFilter(const PxFilterData& filterData, const PxShape* shape, const PxRigidActor* actor, PxHitFlags& queryFlags) override;
		virtual PxQueryHitType::Enum postFilter(const PxFilterData& filterData, const PxQueryHit& hit) override;
	};
    
	class PhysXCCT : public NixApplication {

		PxFoundation*					m_pxFoundation;
		PxPhysics*						m_pxSDK;
		PxPvd*							m_pxPVD;
		PxPvdTransport*					m_transport;
		PxTolerancesScale				m_scale;
		PxCooking*						m_pxCooking;
		PxScene*						m_pxScene;
		PxPvdSceneClient*				m_pxScenePvdClient;
		PxMaterial*						m_pxMaterial;
		PxDefaultCpuDispatcher*			m_cpuDispatcher;
		//
		PxControllerManager*			m_controllerManager;
		PxController*					m_controller;
		//
		NixPhysicsEventCallback			m_simulateEventCallback;
		NixControllerBehaviorCallback	m_controllerBehaviorCallback;
		NixUserControllerHitReport		m_controllerHitReport;
		NixControllerQueryFilterCallback\
										m_controllerQueryFilterCallback;
		PxControllerFilters				m_controllerFilters;

	private:
		std::chrono::time_point<std::chrono::system_clock> m_timePoint;
		//
		virtual bool initialize(void* _wnd, Nix::IArchieve* _archieve );

		virtual void resize(uint32_t _width, uint32_t _height);

		virtual void release();

		virtual void tick();

		virtual const char * title();

		virtual uint32_t rendererType();

		virtual void onMouseEvent(eMouseButton _bt, eMouseEvent _event, int _x, int _y);

		virtual void onKeyEvent(unsigned char _key, eKeyEvent _event);
	};
}

extern Nix::PhysXCCT theapp;
NixApplication* GetApplication();