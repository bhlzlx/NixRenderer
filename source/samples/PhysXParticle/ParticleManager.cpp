#include "ParticleManager.h"
#include "PxPhysicsAPI.h"
#include <cmath>

namespace Nix {

	PxVec3 ParticleEmiter::emit() const {

		static std::default_random_engine random(time(NULL));
		std::uniform_int_distribution<int> dis1(0, 359);

		dis1(random);

		//unsigned int s = (unsigned)time(0);
		//std::uniform_int_distribution<unsigned int> u(0, 359);
		float arc = float(dis1(random)) / 180.0f * 3.141592654f;
		PxVec3 velocity;
		velocity.y = cos(m_emitArc);
		float v = sin(m_emitArc);
		velocity.x = sin(arc) * v;
		velocity.z = cos(arc) * v;
		velocity = velocity.getNormalized();
		velocity *= m_emitVelocity;
		return velocity;
	}

	void ParticleEmiter::onTick() {
		m_vecPosition.clear();
	}
}