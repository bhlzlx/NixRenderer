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
		if (!m_scene) {
			return false;
		}
		m_pvdClient = m_scene->getScenePvdClient();
		if (m_pvdClient) {
			m_pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
			m_pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
			m_pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
		}
		m_physics = _physics;

		PxPhysics* physics = m_physics->getPhysX();
		// 地面材质
		PxMaterial* material = m_physics->getPhysX()->createMaterial(0.5f, 0.5f, 0.6f);
		PxRigidStatic* groundPlane = PxCreatePlane(*m_physics->getPhysX(), PxPlane(0, 1, 0, 0), *material);
		m_scene->addActor(*groundPlane);
		// 添加一个球测试一下
		PxSphereGeometry geometry = PxSphereGeometry(0.5f);
		PxRigidDynamic* rigidBall = PxCreateDynamic(*physics, PxTransform(PxVec3(0, 1, 0)), geometry, *material, 10.0f);
		rigidBall->setAngularDamping(0.5f);
		rigidBall->setLinearVelocity(PxVec3(0, 5, 0));
		m_scene->addActor(*rigidBall);
		//
		return true;
	}

	bool PhysXScene::simulate(float _dt) {
		m_scene->simulate(_dt);
		m_scene->fetchResults(true);

		auto number = m_scene->getNbActors(PxActorTypeFlag::eRIGID_DYNAMIC);
		PxActor* actors;
		m_scene->getActors(PxActorTypeFlag::eRIGID_DYNAMIC, &actors, sizeof(actors));
		PxRigidDynamic* ball = actors->is<PxRigidDynamic>();
		assert(ball);
		PxTransform transform = ball->getGlobalPose();
		m_ballPosition = transform.p;

		return true;
	}

	void PhysXScene::addPlane(float _x, float _y, float _z)
	{
	}

	void PhysXScene::addBall(float _rad, PxVec3 _p, PxVec3 _v)
	{
	}

}