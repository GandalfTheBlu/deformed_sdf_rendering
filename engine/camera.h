#pragma once
#include <matrix.hpp>

namespace Engine
{
	class Camera
	{
	private:
		float fovy, aspect, near, far;
	public:
		Camera();
		~Camera();

		// fovy (vertical fov) in degrees
		void Init(float _fovy, float _aspect, float _near, float _far);

		glm::mat4 CalcV(const glm::mat4& worldMatrix) const;
		glm::mat4 CalcP() const;

		float GetNearPlane() const;
		float GetFarPlane() const;
		float GetFovy() const;
	};
}