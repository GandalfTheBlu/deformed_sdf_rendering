#pragma once
#include <matrix.hpp>
#include <gtx/quaternion.hpp>

namespace Engine
{
	struct Transform
	{
		glm::vec3 position;
		glm::quat rotation;
		float scale;

		Transform();
		Transform(const glm::vec3& _position, const glm::quat& _rotation, float scale);
		
		glm::mat4 Matrix() const;
	};

	Transform Lerp(const Transform& t1, const Transform& t2, float alpha);
	Transform Multiply(const Transform& t1, const Transform& t2);
}