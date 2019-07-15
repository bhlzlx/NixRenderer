#include "PhysXScene.h"
#include "PhysXSystem.h"
#include "PhysXControllerManager.h"
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
		m_scene->setSimulationEventCallback(&m_simulatorCallback);
		if (m_pvdClient) {
			m_pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
			m_pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
			m_pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
		}
		m_physics = _physics;

		PxPhysics* physics = m_physics->getPhysX();
		// �������?
		m_commonMaterial = m_physics->getPhysX()->createMaterial(0.5f, 0.5f, 0.6f);
		PxRigidStatic* groundPlane = PxCreatePlane(*m_physics->getPhysX(), PxPlane(0, 1, 0, 0), *m_commonMaterial);
		PxShape* shape;
		groundPlane->getShapes(&shape, sizeof(PxShape*));
		PxFilterData filterData;
		filterData.word3 = Mesh;
		shape->setSimulationFilterData(filterData);
		m_scene->addActor(*groundPlane);
		// ����һ�������һ��?
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

	Nix::PhysxControllerManager* PhysXScene::createControllerManager()
	{
		PhysxControllerManager* manager = new PhysxControllerManager();
		if (manager->initialize(this)){
			return manager;
		}
		return nullptr;
	}

	void PhysXScene::addParticlePrimitive(const PxVec3& _position, const PxVec3& _velocity)
	{
		auto physx = m_physics->getPhysX();
		PxShape* shape = physx->createShape(PxBoxGeometry(.5f, .5f, .5f), *m_commonMaterial);
		PxRigidDynamic* body = physx->createRigidDynamic(PxTransform(_position));
		body->attachShape(*shape);
		PxRigidBodyExt::updateMassAndInertia(*body, 10.0f);
		//
		shape->release();
		// ����һ�������һ��?
		//PxBoxGeometry geometry = PxBoxGeometry(0.01f, 0.01f, 0.01f);
		//PxSphereGeometry geometry(0.01f);
		//PxRigidDynamic* rigidBall = PxCreateDynamic(*m_physics->getPhysX(), PxTransform(_position), geometry, *m_commonMaterial, 10.0f);
		//rigidBall->setAngularDamping(0.5f);
		body->setLinearVelocity(_velocity);
		
		//PxShape* shape;
		//body->getShapes(&shape, sizeof(shape), 0);
		PxFilterData filterData;
		filterData.word3 = Particle;
		shape->setSimulationFilterData(filterData);
		m_scene->addActor(*body);
	}

	void PhysXScene::addHeightField(uint8_t * _rawData, uint32_t _row, uint32_t _col, PxVec3 _fieldOffset, PxVec3 _scale )
	{
		std::vector<PxHeightFieldSample> samples(_row * _col);
		for (uint32_t r = 0; r < _row; ++r)
		{
			for (uint32_t c = 0; c < _col; ++c)
			{
				uint32_t index = r * _col + c;
				samples[index].height = uint16_t(_rawData[index]);
				if (_row & 0x1) {
					samples[index].clearTessFlag();
				}
				else {
					samples[index].setTessFlag();
				}
				samples[index].materialIndex0 = 0;
				samples[index].materialIndex1 = 0;
			}
		}
		PxHeightFieldDesc hfdesc;
		hfdesc.format = PxHeightFieldFormat::eS16_TM;
		hfdesc.flags = PxHeightFieldFlags(0);
		hfdesc.nbColumns = _col;
		hfdesc.nbRows = _row;
		PxStridedData strideData;
		strideData.data = samples.data();
		strideData.stride = sizeof(PxHeightFieldSample);
		hfdesc.samples = strideData;
		PxHeightField* heightField = m_physics->getCooking()->createHeightField(hfdesc, m_physics->getPhysX()->getPhysicsInsertionCallback());
		
		PxTransform transform(_fieldOffset);
		//PxRigidStatic* hfActor = m_physics->getPhysX()->createRigidStatic(transform);
		PxRigidStatic* hfActor = m_physics->getPhysX()->createRigidStatic(transform);
		if (!hfActor){
			assert(false);
		}
		hfActor->userData = this;
		PxHeightFieldGeometry hfGeom( heightField, PxMeshGeometryFlags(), 1.f / 255.0f * _scale.y, (PxReal)_scale.z, (PxReal)_scale.x);
		PxShape* hfShape = PxRigidActorExt::createExclusiveShape(*hfActor, hfGeom, *m_commonMaterial);
		hfShape->setFlag(PxShapeFlag::eSCENE_QUERY_SHAPE, true);
		if (!hfShape){
			assert(false);
		}
		PxFilterData hfFilterData;
		hfFilterData.word3 = Mesh;
		hfShape->setSimulationFilterData(hfFilterData);
		m_scene->addActor(*hfActor);
	}

	void PhysXScene::getParticlePrimitivePositions(std::vector<PxVec3>& _positions)
	{
		static std::vector< PxActor* > actors;
		actors.resize(m_scene->getNbActors(PxActorTypeFlag::eRIGID_DYNAMIC));
		m_scene->getActors( PxActorTypeFlag::eRIGID_DYNAMIC, actors.data(), actors.size() * sizeof(PxActor*) );
		for (auto actor : actors) {
			PxRigidDynamic* rigid = actor->is<PxRigidDynamic>();
			
			if (rigid) {
				const bool sleeping = rigid->isSleeping();
				if (sleeping) {
					PxShape* shape = nullptr;
					rigid->getShapes(&shape, sizeof(shape), 0);
					if (shape->getSimulationFilterData().word3 == Particle) {
						rigid->release();
					}
				}
				else {
					_positions.push_back(rigid->getGlobalPose().p);
				}
				
			}
		}
	}

	bool PhysXScene::raycast(const PxVec3& _start, const PxVec3& _direction, float _distance, PxVec3& _position)
	{
		PxRaycastBuffer rayHit;

		// we dont use query filter currently
		PxQueryFilterData filterData;
		PxQueryFilterCallback* cb = nullptr;
		if (!m_scene->raycast(
			_start, _direction, _distance,
			rayHit,
			PxHitFlag::ePOSITION,
			filterData,
			cb
		)) {
			// û���κ��ཻ������ʧ�ܣ�
			return false;
		}
		// �ӱ����ǿ���������
		//hit.pPhysicsActor = (PhysicsActor*)rayHit.block.actor->userData;
		//hit.position = PxVec3ToGxVec3(rayHit.block.position);
		//hit.distance = rayHit.block.distance;
		//hit.normal = PxVec3ToGxVec3(rayHit.block.normal);
		_position = rayHit.block.position;
		return true;
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