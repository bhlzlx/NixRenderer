#pragma once
#include "PxPhysicsAPI.h"
#include <vector>
#include <ctime>
#include <random>

namespace Nix {

	using namespace physx;

	class ParticleEmiter {
	private:
		PxVec3		m_emitSource;
		float		m_emitArc;
		float		m_emitVelocity;
		//
		std::default_random_engine m_re;
		//
		std::vector<PxVec3>	m_vecPosition;
	public:
		ParticleEmiter(const PxVec3& _source, float _arc, float _velocity)
			: m_emitSource(_source)
			, m_emitArc(_arc)
			, m_emitVelocity(_velocity)
		{
		}
		// generate particle with a velocity
		PxVec3 emit() const;
		// reset particle list when tick
		void onTick();
	};

}