#include "PhysXControllerManager.h"
#include "PhysXScene.h"
#include "PhysXSystem.h"

namespace Nix {


	PX_INLINE void addForceAtPosInternal(PxRigidBody& body, const PxVec3& force, const PxVec3& pos, PxForceMode::Enum mode, bool wakeup)
	{
		/*	if(mode == PxForceMode::eACCELERATION || mode == PxForceMode::eVELOCITY_CHANGE)
		{
		Ps::getFoundation().error(PxErrorCode::eINVALID_PARAMETER, __FILE__, __LINE__,
		"PxRigidBodyExt::addForce methods do not support eACCELERATION or eVELOCITY_CHANGE modes");
		return;
		}*/

		const PxTransform globalPose = body.getGlobalPose();
		const PxVec3 centerOfMass = globalPose.transform(body.getCMassLocalPose().p);

		const PxVec3 torque = (pos - centerOfMass).cross(force);
		body.addForce(force, mode, wakeup);
		body.addTorque(torque, mode, wakeup);
	}


	static void addForceAtLocalPos(PxRigidBody& body, const PxVec3& force, const PxVec3& pos, PxForceMode::Enum mode, bool wakeup = true)
	{
		//transform pos to world space
		const PxVec3 globalForcePos = body.getGlobalPose().transform(pos);

		addForceAtPosInternal(body, force, globalForcePos, mode, wakeup);
	}


	void defaultCCTInteraction(const PxControllerShapeHit& hit)
	{
		PxRigidDynamic* actor = hit.shape->getActor()->is<PxRigidDynamic>();
		if (actor)
		{
			if (actor->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC)
				return;

			if (0)
			{
				const PxVec3 p = actor->getGlobalPose().p + hit.dir * 10.0f;

				PxShape* shape;
				actor->getShapes(&shape, 1);
				PxRaycastHit newHit;
				PxU32 n = PxShapeExt::raycast(*shape, *shape->getActor(), p, -hit.dir, 20.0f, PxHitFlag::ePOSITION, 1, &newHit);
				if (n)
				{
					// We only allow horizontal pushes. Vertical pushes when we stand on dynamic objects creates
					// useless stress on the solver. It would be possible to enable/disable vertical pushes on
					// particular objects, if the gameplay requires it.
					const PxVec3 upVector = hit.controller->getUpDirection();
					const PxF32 dp = hit.dir.dot(upVector);
					//		shdfnd::printFormatted("%f\n", fabsf(dp));
					if (fabsf(dp) < 1e-3f)
						//		if(hit.dir.y==0.0f)
					{
						const PxTransform globalPose = actor->getGlobalPose();
						const PxVec3 localPos = globalPose.transformInv(newHit.position);
						addForceAtLocalPos(*actor, hit.dir*hit.length*1000.0f, localPos, PxForceMode::eACCELERATION);
					}
				}
			}

			// We only allow horizontal pushes. Vertical pushes when we stand on dynamic objects creates
			// useless stress on the solver. It would be possible to enable/disable vertical pushes on
			// particular objects, if the gameplay requires it.
			const PxVec3 upVector = hit.controller->getUpDirection();
			const PxF32 dp = hit.dir.dot(upVector);
			//		shdfnd::printFormatted("%f\n", fabsf(dp));
			if (fabsf(dp) < 1e-3f)
				//		if(hit.dir.y==0.0f)
			{
				const PxTransform globalPose = actor->getGlobalPose();
				const PxVec3 localPos = globalPose.transformInv(toVec3(hit.worldPos));
				addForceAtLocalPos(*actor, hit.dir*hit.length*1000.0f, localPos, PxForceMode::eACCELERATION);
			}
		}
	}

    
	bool PhysxControllerManager::initialize(PhysXScene* _scene) {
		m_scene = _scene;
        m_manager = PxCreateControllerManager( *m_scene->getScene(), true);
        return !!m_manager;
    }

	PhysxCapsuleController * PhysxControllerManager::createController(){
		PxCapsuleControllerDesc desc = {};
		desc.material = this->m_scene->getSystem()->getMaterial();
		desc.height = 1.0f;
		desc.radius = 0.2f;
		desc.behaviorCallback = this;
		desc.climbingMode = PxCapsuleClimbingMode::eCONSTRAINED;
		desc.nonWalkableMode = PxControllerNonWalkableMode::ePREVENT_CLIMBING;
		desc.reportCallback = this;
		//desc.
        PxController* controller = this->m_manager->createController(desc);		

		PxRigidDynamic* actor = controller->getActor();
		PxShape* shape;
		if (actor)
		{
			if (actor->getNbShapes())
			{
				PxShape* ctrlShape;
				actor->getShapes(&ctrlShape, 1);
				ctrlShape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, true);
				ctrlShape->setFlag(PxShapeFlag::eSCENE_QUERY_SHAPE, true);
				ctrlShape->setFlag(PxShapeFlag::eVISUALIZATION, true);
			}
		}

		PhysxCapsuleController* c = new PhysxCapsuleController( controller );
		controller->setStepOffset(0.2f);
		//PxObstacleContext* obstacleContext = m_manager->createObstacleContext();
		//c->setObstacle(obstacleContext);
		//controller->getActor()->getShapes()
		controller->setUpDirection(PxVec3(0.0f, 1.0f, 0.0f));
		return c;
    }

	PhysxControllerManager::~PhysxControllerManager(){

	}

	void PhysxControllerManager::onShapeHit(const PxControllerShapeHit& hit)
	{
		defaultCCTInteraction(hit);
	}

	PhysxCapsuleController::PhysxCapsuleController(PxController* _controller)
		: m_controller(_controller)
	{
		m_obstacle.mHalfHeight = 0.5f;
		m_obstacle.mRadius = 0.2;
		m_filters.mFilterData = &m_filterData;
		m_filters.mFilterCallback = this;
		m_filterData.word3 = ObjectType::Controller;
		m_minDist = 0.0f;
	}

	uint32_t PhysxCapsuleController::move(const PxVec3& _displacement, PxF32 _elapseTime)
	{
		auto flag = m_controller->move(_displacement, m_minDist, _elapseTime, m_filters, nullptr);
		uint32_t v = flag.operator uint32_t();
		/*m_obstacle.mPos = m_controller->getPosition();
		m_obstacleConetxt->updateObstacle(m_obstacleHandle, m_ob
		m_obstacle.mPos.y -= 1.0f;stacle);*/
		return v;
		// 		PxControllerCollisionFlags collisionFlags =
		// 			PxController::move(const PxVec3& disp, PxF32 minDist, PxF32 elapsedTime,
		// 				const PxControllerFilters& filters, const PxObstacleContext* obstacles = NULL);
	}

	physx::PxVec3 PhysxCapsuleController::getPosition()
	{
		const auto& posExt = m_controller->getPosition();
		return PxVec3(posExt.x, posExt.y, posExt.z);
	}

	void PhysxCapsuleController::setPosition(const PxVec3& _position)
	{
		m_controller->setPosition(PxExtendedVec3(_position.x, _position.y, _position.z));
		//m_obstacle.mPos = PxExtendedVec3(_position.x, _position.y - 1, _position.z);
		//m_obstacleConetxt->updateObstacle(m_obstacleHandle, m_obstacle);
	}

	inline void PhysxCapsuleController::setObstacle(PxObstacleContext* _obstacle) {
		m_obstacleConetxt = _obstacle;
		m_obstacleHandle = m_obstacleConetxt->addObstacle(m_obstacle);
	}

	physx::PxQueryHitType::Enum PhysxCapsuleController::preFilter(const PxFilterData& filterData, const PxShape* shape, const PxRigidActor* actor, PxHitFlags& queryFlags)
	{
		return PxQueryHitType::Enum::eBLOCK;
	}

	physx::PxQueryHitType::Enum PhysxCapsuleController::postFilter(const PxFilterData& filterData, const PxQueryHit& hit)
	{
		return PxQueryHitType::Enum::eBLOCK;
		//throw std::logic_error("The method or operation is not implemented.");
	}

}