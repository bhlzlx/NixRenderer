#include "PhysXControllerManager.h"
#include "PhysXScene.h"
#include "PhysXSystem.h"

namespace Nix {
    
	bool PhysxControllerManager::initialize(PhysXScene* _scene) {
		m_scene = _scene;
        m_manager = PxCreateControllerManager( *m_scene->getScene() );
        return !!m_manager;
    }

	PhysxCapsuleController * PhysxControllerManager::createController(){
		PxCapsuleControllerDesc desc;
		desc.material = this->m_scene->getSystem()->getMaterial();
		desc.height = 1.0f;
		desc.radius = 0.2f;
        PxController* controller = this->m_manager->createController(desc);
		
		PhysxCapsuleController* c = new PhysxCapsuleController( controller );
		controller->setStepOffset(0.2f);
		PxObstacleContext* obstacleContext = m_manager->createObstacleContext();
		c->setObstacle(obstacleContext);
		//controller->getActor()->getShapes()
		controller->setUpDirection(PxVec3(0.0f, 1.0f, 0.0f));
		return c;
    }

	PhysxControllerManager::~PhysxControllerManager(){

	}

	PhysxCapsuleController::PhysxCapsuleController(PxController* _controller) 
		: m_controller(_controller)
	{
		m_obstacle.mHalfHeight = 0.5f;
		m_obstacle.mRadius = 0.2;
		m_filters.mFilterData = &m_filterData;
		m_filterData.word3 = ObjectType::Controller;
		m_minDist = 0.1f;
	}

	uint32_t PhysxCapsuleController::move(const PxVec3& _displacement, PxF32 _elapseTime)
	{
		auto flag = m_controller->move(_displacement, m_minDist, _elapseTime, m_filters, m_obstacleConetxt);
		uint32_t v = flag.operator uint32_t();
		//m_obstacle.mPos = m_controller->getPosition();
		//m_obstacleConetxt->updateObstacle(m_obstacleHandle, m_obstacle);
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
		m_obstacle.mPos = PxExtendedVec3(_position.x, _position.y, _position.z);
		m_obstacleConetxt->updateObstacle(m_obstacleHandle, m_obstacle);
	}

	inline void PhysxCapsuleController::setObstacle(PxObstacleContext* _obstacle) {
		m_obstacleConetxt = _obstacle;
		m_obstacleHandle = m_obstacleConetxt->addObstacle(m_obstacle);
	}

}