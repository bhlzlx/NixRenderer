#pragma once
#include <PxPhysicsAPI.h>
#include <PxFoundation.h>

namespace Nix {

	using namespace physx;

	class PhysXSystem;

	class PhysXScene {
	private:
		PhysXSystem*			m_physics;
		PxScene*				m_scene;
		PxPvdSceneClient*		m_pvdClient;
	public:
		PxVec3					m_ballPosition;
	public:
		PhysXScene();
		~PhysXScene();
		bool initialize(PhysXSystem* _physics, const PxSceneDesc& _desc);
		bool simulate(float _dt);
		void addPlane( float _x, float _y, float _z );
		void addBall( float _rad, PxVec3 _p, PxVec3 _v );
	};
}