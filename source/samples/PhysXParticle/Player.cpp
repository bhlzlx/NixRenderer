#include "Player.h"
#include "PhysXScene.h"

namespace Nix {
	
	void Player::move(const PxVec3& _direction, float _dt)
	{
		m_velocity = _direction * 5;// 一秒5个单位算吧
		m_velocity.y = 0.0f;
	}

	void Player::tick(float _dt)
	{
		PxVec3 disp = m_velocity*_dt;
		disp.y -= _dt * 10.0f;
		//PxVec3 ddd(0.0f, -0.1f, 0.0f);
		auto flag = m_controller->move(disp, _dt);
		if (flag & CollideBottom) {
			m_walking = true;
			if (m_velocity.y < 0) {
				m_velocity.y = 0;
			}
		}
		else {
			if (flag & CollideTop) {
				if (m_velocity.y > 0) {
					m_velocity.y = 0;
				}
			}
			m_walking = false;
		}
		m_velocity.x = m_velocity.y = m_velocity.z = 0.0f;
		m_position = m_controller->getPosition();
	}

	void Player::initialize(PhysxControllerManager* _ctmanager, const PxVec3& _position)
	{
		m_controller = _ctmanager->createController();
		m_controller->setPosition(_position);
		m_walking = false;
	}

	const PxVec3& Player::getPosition() const
	{
		return m_position;
	}
}