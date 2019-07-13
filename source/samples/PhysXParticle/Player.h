#include "PhysXControllerManager.h"

namespace Nix {

	class PhysXScene;

    class Player {
    private:
        PhysxCapsuleController*     m_controller;
        bool                        m_walking = false;
        PxVec3                      m_velocity;
		PxVec3						m_position;
    public:
        void move( const PxVec3& _direction, float _dt );
		void tick(float _dt);
		void initialize( PhysxControllerManager* _ctmanager, const PxVec3& _position );
		const PxVec3& getPosition() const;
    };

}