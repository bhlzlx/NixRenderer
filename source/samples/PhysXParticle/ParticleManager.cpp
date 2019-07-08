#include "ParticleManager.h"
#include <cmath>

namespace Nix {

	glm::vec3 ParticleEmiter::emit() const {
		unsigned int s = (unsigned)time(0);
		std::uniform_int_distribution<unsigned> u(0, 359);
		float arc = float(u(m_re)) / 180.0f * 3.141592654f;
		glm::vec3 velocity;
		velocity.y = cos(m_emitArc);
		float v = sin(m_emitArc);
		velocity.x = sin(arc) * v;
		velocity.z = cos(arc) * v;
		velocity = glm::normalize(velocity);
		velocity *= m_emitVelocity;
		return velocity;
	}

	void ParticleEmiter::onTick() {

	}

	void ParticleEmiter::updatePrimitive(const glm::vec3& _position) {

	}

}