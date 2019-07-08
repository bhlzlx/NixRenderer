#pragma once
#include <glm/glm.hpp>
#include <MemoryPool.h>
#include <vector>
#include <ctime>
#include <random>

namespace Nix {

	class ParticleEmiter {
	private:
		glm::vec3	m_emitSource;
		float		m_emitArc;
		float		m_emitVelocity;
		//
		std::default_random_engine m_re;
		//
		std::vector<glm::vec3>	m_vecPosition;
	public:
		ParticleEmiter(const glm::vec3& _source, float _arc, float _velocity)
			: m_emitSource(glm::vec3(0))
			, m_emitArc(60.0f)
			, m_emitVelocity(6.0f)
		{
		}

		glm::vec3 emit() const;
		void onTick();
		void updatePrimitive( const glm::vec3& _position );
	};

}