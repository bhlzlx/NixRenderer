#pragma once
#include <PxPhysicsAPI.h>
#include <PxFoundation.h>
#include <vector>

namespace Nix {

	enum ObjectType {
		Particle =	1,
		Mesh =		1 << 1
	};

	using namespace physx;

	class PhysXScene;

	class NixSimulationCallback : public PxSimulationEventCallback {
	private:
		PhysXScene* m_scene;
	public:
		virtual void onConstraintBreak(PxConstraintInfo* constraints, PxU32 count) override;
		virtual void onWake(PxActor** actors, PxU32 count) override;
		virtual void onSleep(PxActor** actors, PxU32 count) override;
		virtual void onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs) override;
		virtual void onTrigger(PxTriggerPair* pairs, PxU32 count) override;
		virtual void onAdvance(const PxRigidBody*const* bodyBuffer, const PxTransform* poseBuffer, const PxU32 count) override;
	};

	class PhysXSystem;

	class PhysXScene {
	private:
		PhysXSystem*			m_physics;
		PxScene*				m_scene;
		PxPvdSceneClient*		m_pvdClient;
		PxMaterial*				m_commonMaterial;
		NixSimulationCallback	m_simulatorCallback;
	public:
	public:
		PhysXScene();
		~PhysXScene();
		bool initialize(PhysXSystem* _physics, const PxSceneDesc& _desc);
		bool simulate(float _dt);
		//
		void addParticlePrimitive(const PxVec3& _position, const PxVec3& _velocity);
		void addHeightField(uint8_t * _rawData, uint32_t _row, uint32_t _col, PxVec2 _fieldOffset, float _cellSize);
		void getParticlePrimitivePositions( std::vector<PxVec3>& _positions );
	};
}