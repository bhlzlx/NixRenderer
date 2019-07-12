#include "PhysXControllerManager.h"

namespace Nix {

    class Player {
    private:
        PhysxCapsuleController*     m_controller;
        bool                        m_walking = false;
        PxVec3                      m_velocity;
    public:
        void move();
    };

}