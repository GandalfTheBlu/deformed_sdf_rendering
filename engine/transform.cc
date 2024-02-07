#include "transform.h"

namespace Engine
{
	Transform::Transform() :
		position(0.f),
		rotation(glm::identity<glm::quat>()),
		scale(1.f)
	{}

	Transform::Transform(const glm::vec3& _position, const glm::quat& _rotation, float _scale) :
		position(_position),
		rotation(_rotation),
		scale(_scale)
	{}

	glm::mat4 Transform::Matrix() const
	{
		glm::mat3 RS = glm::mat3_cast(rotation) * scale;

		return glm::mat4(
			glm::vec4(RS[0], 0.f),
			glm::vec4(RS[1], 0.f),
			glm::vec4(RS[2], 0.f),
			glm::vec4(position, 1.f)
		);
	}

	Transform Lerp(const Transform& t1, const Transform& t2, float alpha)
	{
		return Transform(
			glm::mix(t1.position, t2.position, alpha), 
			glm::slerp(t1.rotation, t2.rotation, alpha), 
			glm::mix(t1.scale, t2.scale, alpha)
		);
	}

	Transform Multiply(const Transform& t1, const Transform& t2)
	{
		return Transform(
			t1.position + t1.rotation * (t1.scale * t2.position),
			t1.rotation * t2.rotation,
			t1.scale * t2.scale
		);
	}
}