#include "PhysXScene.h"
#include "PhysXSystem.h"
#include <assert.h>

namespace Nix {

	PhysXScene::PhysXScene()
	{

	}

	PhysXScene::~PhysXScene()
	{

	}

	bool PhysXScene::initialize(PhysXSystem* _physics, const PxSceneDesc& _desc)
	{
		auto physX = _physics->getPhysX();
		m_scene = physX->createScene(_desc);
		//m_scene->setContactModifyCallback()
		if (!m_scene) {
			return false;
		}
		m_pvdClient = m_scene->getScenePvdClient();
		//m_scene->setSimulationEventCallback()
		if (m_pvdClient) {
			m_pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
			m_pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
			m_pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
		}
		m_physics = _physics;

		PxPhysics* physics = m_physics->getPhysX();
		// 地面材质
		m_commonMaterial = m_physics->getPhysX()->createMaterial(0.5f, 0.5f, 0.6f);
		PxRigidStatic* groundPlane = PxCreatePlane(*m_physics->getPhysX(), PxPlane(0, 1, 0, 0), *m_commonMaterial);
		m_scene->addActor(*groundPlane);
		// 添加一个球测试一下
// 		PxSphereGeometry geometry = PxSphereGeometry(0.5f);
// 		PxRigidDynamic* rigidBall = PxCreateDynamic(*physics, PxTransform(PxVec3(0, 1, 0)), geometry, *m_commonMaterial, 10.0f);
// 		rigidBall->setAngularDamping(0.5f);
// 		rigidBall->setLinearVelocity(PxVec3(0, 5, 0));
// 		PxShape* shape;
// 		PxFilterData filterData;
// 		filterData.word0 = Particle;
// 		shape->setSimulationFilterData(filterData);
// 		m_scene->addActor(*rigidBall);
		//
		return true;
	}

	bool PhysXScene::simulate(float _dt) {
		m_scene->simulate(_dt);
		m_scene->fetchResults(true);

// 		auto number = m_scene->getNbActors(PxActorTypeFlag::eRIGID_DYNAMIC);
// 		PxActor* actors;
// 		PxU32 count = m_scene->getActors(PxActorTypeFlag::eRIGID_DYNAMIC, &actors, sizeof(actors));
// 		PxRigidDynamic* ball = actors->is<PxRigidDynamic>();
// 		assert(ball);
// 		PxTransform transform = ball->getGlobalPose();
// 		m_ballPosition = transform.p;

		return true;
	}

	void PhysXScene::addParticlePrimitive(const PxVec3& _position, const PxVec3& _velocity)
	{
		// 添加一个球测试一下
		PxBoxGeometry geometry = PxBoxGeometry(0.1f, 0.1f, 0.1f);
		PxRigidDynamic* rigidBall = PxCreateDynamic(*m_physics->getPhysX(), PxTransform(_position), geometry, *m_commonMaterial, 10.0f);
		rigidBall->setAngularDamping(0.5f);
		rigidBall->setLinearVelocity(_velocity);
		
		PxShape* shape;
		rigidBall->getShapes(&shape, sizeof(shape), 0);
		PxFilterData filterData;
		filterData.word0 = Particle;
		//shape->setSimulationFilterData(filterData);
		m_scene->addActor(*rigidBall);
	}

	void PhysXScene::getParticlePrimitivePositions(std::vector<PxVec3>& _positions)
	{
		static std::vector< PxActor* > actors;
		actors.resize(m_scene->getNbActors(PxActorTypeFlag::eRIGID_DYNAMIC));
		m_scene->getActors( PxActorTypeFlag::eRIGID_DYNAMIC, actors.data(), actors.size() * sizeof(PxActor*) );
		for (auto actor : actors) {
			PxRigidDynamic* rigid = actor->is<PxRigidDynamic>();
			if (rigid) {
				_positions.push_back(rigid->getGlobalPose().p);
			}
		}
	}

	void NixSimulationCallback::onConstraintBreak(PxConstraintInfo* constraints, PxU32 count)
	{
		return;
	}

	void NixSimulationCallback::onWake(PxActor** actors, PxU32 count)
	{
		return;
	}

	void NixSimulationCallback::onSleep(PxActor** actors, PxU32 count)
	{
		// recycle the particle object
		for (PxU32 index = 0; index < count; ++index) {
			PxActor * actor = actors[index];
			PxRigidDynamic* rigid = actor->is<PxRigidDynamic>();
			// get only one shape from rigid dynamic, just for test
			PxShape* shape = nullptr;
			rigid->getShapes(&shape, sizeof(shape), 0);
			if (shape->getSimulationFilterData().word0 == Particle) {
				rigid->release();
			}
		}
	}

	void NixSimulationCallback::onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs)
	{
		// will not handle it
	}

	void NixSimulationCallback::onTrigger(PxTriggerPair* pairs, PxU32 count)
	{
		// will not handle it
	}

	void NixSimulationCallback::onAdvance(const PxRigidBody*const* bodyBuffer, const PxTransform* poseBuffer, const PxU32 count)
	{
		// will not handle it
	}

}