#pragma once
#include <PxPhysicsAPI.h>
#include <PxFoundation.h>
#include <vector>

namespace Nix {

    using namespace physx;

	class PhysXScene;

    class PhysxCapsuleController{
    private:
		PxController*           m_controller;
		PxVec3*					m_disp;			// gravity and other force
		PxF32					m_minDist;		// 

		PxFilterData			m_filterData;
		PxControllerFilters		m_filters;
		PxObstacleContext*		m_obstacleConetxt;
		PxCapsuleObstacle		m_obstacle;
		ObstacleHandle			m_obstacleHandle;
    public:
		PhysxCapsuleController( PxController* _controller );
		uint32_t move( const PxVec3& _displacement, PxF32 _elapseTime );
		PxVec3 getPosition();
		void setPosition( const PxVec3& _position );
		void setObstacle(PxObstacleContext* _obstacle);
    };

    class PhysxControllerManager {
        private:
            PhysXScene*				m_scene;
            PxControllerManager*    m_manager;
        public:
        PhysxControllerManager() 
        : m_scene( nullptr ) {
        }

        bool initialize(PhysXScene* _scene );
      
		PhysxCapsuleController * createController();

        ~PhysxControllerManager();
    };

}
