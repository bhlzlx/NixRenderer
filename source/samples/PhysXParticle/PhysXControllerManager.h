#include <PxPhysicsAPI.h>
#include <PxFoundation.h>
#include <vector>

namespace Nix {

    using namespace physx;

    class PhysxCapsuleController{
    private:
		PxController*           m_controller;
		PxVec3*					m_disp;			// gravity and other force
		PxF32					m_minDist;		// 

		PxFilterData			m_filterData;
		PxControllerFilters		m_filters;
    public:
		PhysxCapsuleController( PxController* _controller );
		uint32_t move( const PxVec3& _displacement, PxF32 _elapseTime );
		PxVec3 getPosition();
		void setPosition( const PxVec3& _position );

    };

    class PhysxControllerManager {
        private:
            PxScene*                m_scene;
            PxControllerManager*    m_manager;
        public:
        PhysxControllerManager( PxScene* _scene ) 
        : m_scene(_scene) {
        }
        bool initialize();
        
		PhysxCapsuleController * createController();

        ~PhysxControllerManager();
    };

}
