#include "camera.h"
#include <gtc/matrix_transform.hpp>

namespace Engine
{
	Camera::Camera() :
		fovy(0.f),
		aspect(0.f),
		near(0.f),
		far(0.f)
	{}

	Camera::~Camera()
	{}

	void Camera::Init(float _fovy, float _aspect, float _near, float _far)
	{
		fovy = _fovy;
		aspect = _aspect;
		near = _near;
		far = _far;
	}

	glm::mat4 Camera::CalcV(const glm::mat4& worldMatrix) const
	{
		glm::vec3 z = -worldMatrix[2];
		glm::vec3 x = glm::cross(z, glm::vec3(worldMatrix[1]));
		glm::vec3 y = glm::cross(x, z);
		glm::vec3 p = worldMatrix[3];

		return glm::mat4(
			glm::vec4(x.x, y.x, z.x, 0.f),
			glm::vec4(x.y, y.y, z.y, 0.f),
			glm::vec4(x.z, y.z, z.z, 0.f),
			glm::vec4(-glm::dot(x, p), -glm::dot(y, p), -glm::dot(z, p), 1.f)
		);
	}

	glm::mat4 Camera::CalcP() const
	{
		return glm::perspective(fovy, aspect, near, far);
	}

	float Camera::GetNearPlane() const
	{
		return near;
	}

	float Camera::GetFarPlane() const
	{
		return far;
	}

	float Camera::GetFovy() const
	{
		return fovy;
	}
}