#pragma once
#include <PxPhysicsAPI.h>
#include <PxFoundation.h>
#include <vector>

namespace Nix {

    using namespace physx;

	class PhysXScene;

    class PhysxCapsuleController 
		: PxQueryFilterCallback
		
	{
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


		virtual PxQueryHitType::Enum preFilter(const PxFilterData& filterData, const PxShape* shape, const PxRigidActor* actor, PxHitFlags& queryFlags) override;


		virtual PxQueryHitType::Enum postFilter(const PxFilterData& filterData, const PxQueryHit& hit) override;

	};

    class PhysxControllerManager 
		: public PxControllerBehaviorCallback
		, public PxUserControllerHitReport
	{
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

		//
		//////////////////////////////////////////////////////////////////////////

		virtual void							onShapeHit(const PxControllerShapeHit& hit);
		virtual void							onControllerHit(const PxControllersHit& hit) {}
		virtual void							onObstacleHit(const PxControllerObstacleHit& hit) {}

		virtual PxControllerBehaviorFlags getBehaviorFlags(const PxShape& shape, const PxActor& actor)
		{
			const char* actorName = actor.getName();
#ifdef PLATFORMS_AS_OBSTACLES
			PX_ASSERT(actorName != gPlatformName);	// PT: in this mode we should have filtered out those guys already
#endif

													// PT: ride on planks
													// 			if (actorName == gPlankName)
													// 				return PxControllerBehaviorFlag::eCCT_CAN_RIDE_ON_OBJECT
													// 
													// 			// PT: ride & slide on platforms
													// 			if (actorName == gPlatformName)
													// 				return PxControllerBehaviorFlag::eCCT_CAN_RIDE_ON_OBJECT | PxControllerBehaviorFlag::eCCT_SLIDE;

			return PxControllerBehaviorFlag::eCCT_CAN_RIDE_ON_OBJECT | PxControllerBehaviorFlag::eCCT_SLIDE;
		}

		virtual PxControllerBehaviorFlags getBehaviorFlags(const PxController&)
		{
			return PxControllerBehaviorFlags(0);
		}

		virtual PxControllerBehaviorFlags getBehaviorFlags(const PxObstacle&)
		{
			return PxControllerBehaviorFlag::eCCT_CAN_RIDE_ON_OBJECT | PxControllerBehaviorFlag::eCCT_SLIDE;
		}

	};

}
