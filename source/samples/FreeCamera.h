#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Nix
{
	class FreeCamera
	{
	private:
		// directions
		glm::vec3		forward;
		glm::vec3		topward;
		glm::vec3		leftward;
		// view param
		glm::vec3		eye;
		glm::vec3		lookAt;
		bool			matViewDirty;
		//
		float			vpFar;
		float			vpNear;
		// projection param
		union
		{
			struct { float fov, aspect; };
			struct { float left, right, bottom, top; };
		};
		// projection type
		enum ProjectionType
		{
			ProjectionTypeOrtho,
			ProjectionTypePerpective
		}				projType;;
		bool			matProjDirty;
		// final matrices
		glm::mat4x4		matView;
		glm::mat4x4		matProj;

	public:
		FreeCamera();

		~FreeCamera();

		void SetEye(glm::vec3 _eye);

		void SetLookAt(glm::vec3 _lookAt);
		const glm::vec3& GetLookAt() const;

		const glm::vec3&  GetForword() const {
			return forward;
		}

		void SetTop(glm::vec3 _top);

		void SetMatView(glm::vec3 _eye, glm::vec3 _lookAt, glm::vec3 _top);

		void Tick();

		void SetProjectionType(ProjectionType _type);

		void Perspective(float _fov, float _aspect, float _near, float _far);

		void Ortho(float _left, float _right, float _bottom, float _top, float _near, float _far);

		void Forward(float _distance);

		void Backward(float _distance);

		void Leftward(float _distance);

		void Rightward(float _distance);

		void RotateAxisY(float _angle);

		void RotateAxisX(float _angle);


		const glm::mat4x4& GetViewMatrix();

		const glm::mat4x4& GetProjMatrix();

		const glm::vec3& GetEye();

	};

}