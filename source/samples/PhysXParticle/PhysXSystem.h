#pragma once
#include <PxPhysicsAPI.h>
#include <PxFoundation.h>
#include <extensions/PxDefaultCpuDispatcher.h>
#include <PxFiltering.h>

namespace Nix {
	using namespace physx;
	class PhysXScene;

	class PhysXSystem {
	private:
		PxFoundation*			m_foundation;
		PxPvd*					m_pvd;
		PxPvdTransport*			m_transport;
		PxTolerancesScale		m_scale;
		PxPhysics*				m_physics;
		PxCooking*				m_cooking;
		PxMaterial*				m_material;
		//
		PxDefaultCpuDispatcher*	m_cpuDispatcher;
		//
		PxErrorCallback*		m_errorCallback;
		PxAllocatorCallback*	m_allocCallback;
	public:
		PhysXSystem();
		~PhysXSystem();
		//
		bool initialize();
		void shutdown();

		PxPhysics* getPhysX() {
			return m_physics;
		}
		PxCooking* getCooking() {
			return m_cooking;
		}

		PhysXScene* createScene();
	};

}