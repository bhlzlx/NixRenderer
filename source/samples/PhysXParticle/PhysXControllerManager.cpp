#include "PhysXControllerManager.h"
#include "PhysXScene.h"

namespace Nix {
    
    bool PhysxControllerManager::initialize(){
        m_manager = PxCreateControllerManager( *m_scene );
        return !!m_manager;
    }

	PhysxCapsuleController * PhysxControllerManager::createController(){
		PxCapsuleControllerDesc desc;
		desc.height = 1.0f;
		desc.radius = 0.2f;
        PxController* controller = this->m_manager->createController(desc);
		PhysxCapsuleController* c = new PhysxCapsuleController( controller );
		controller->setStepOffset(0.2f);
		//controller->getActor()->getShapes()
		controller->setUpDirection(PxVec3(0.0f, 1.0f, 0.0f));
		return c;
    }

	PhysxControllerManager::~PhysxControllerManager(){

	}

	PhysxCapsuleController::PhysxCapsuleController(PxController* _controller) : m_controller(_controller)
	{
		m_filters.mFilterData = &m_filterData;
		m_filterData.word3 = ObjectType::Controller;
	}

	uint32_t PhysxCapsuleController::move(const PxVec3& _displacement, PxF32 _elapseTime)
	{
		return m_controller->move(_displacement, m_minDist, _elapseTime, m_filters);
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

	}

}