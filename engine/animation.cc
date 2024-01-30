#include "animation.h"
#include <gtx/quaternion.hpp>

namespace Engine
{
	CapsulePrimitive::CapsulePrimitive() :
		startPoint(0.f),
		endPoint(0.f, 1.f, 0.f),
		radius(0.5f)
	{}

	CapsulePrimitive::CapsulePrimitive(const glm::vec3& _startPoint, const glm::vec3& _endPoint, float _radius) :
		startPoint(_startPoint),
		endPoint(_endPoint),
		radius(_radius)
	{}


	Transform::Transform() :
		position(0.f),
		eulerAngles(0.f),
		scale(1.f)
	{}

	Transform::Transform(const glm::vec3& _position, const glm::vec3& _eulerAngles, const glm::vec3& _scale) :
		position(_position),
		eulerAngles(_eulerAngles),
		scale(_scale)
	{}

	glm::mat4 Transform::Matrix() const
	{
		glm::mat4 T(1.f);

		T[0][0] = scale.x;
		T[1][1] = scale.y;
		T[2][2] = scale.z;

		T = glm::mat4_cast(glm::quat(eulerAngles)) * T;

		T[3] = glm::vec4(position, 1.f);

		return T;
	}


	Bone::Bone() :
		p_parent(nullptr)
	{}

	Bone::Bone(Bone* _p_parent) :
		p_parent(_p_parent)
	{}

	Bone::~Bone()
	{
		for (Bone* p_bone : children)
			delete p_bone;
	}

	void Bone::AddChild()
	{
		children.push_back(new Bone(this));
	}

	void Bone::RemoveChild(size_t childIndex)
	{
		delete children[childIndex];
		children.erase(children.begin() + childIndex);
	}

	size_t Bone::ChildrenCount()
	{
		return children.size();
	}

	Bone& Bone::GetChild(size_t childIndex)
	{
		return *children[childIndex];
	}

	void Bone::GenerateBindPose(SkeletonBindPose& outData, const glm::mat4& parentWorldTransform)
	{
		glm::mat4 worldTransform = parentWorldTransform * localTransform.Matrix();
		glm::vec3 worldStartPoint = glm::vec3(worldTransform * glm::vec4(localWeightVolume.startPoint, 1.f));
		glm::vec3 worldEndPoint = glm::vec3(worldTransform * glm::vec4(localWeightVolume.endPoint, 1.f));

		outData.inverseWorldTransforms.push_back(glm::inverse(worldTransform));
		outData.weightVolumes.emplace_back(worldStartPoint, worldEndPoint, localWeightVolume.radius);

		for (Bone* p_child : children)
			p_child->GenerateBindPose(outData, worldTransform);
	}

	void Bone::GenerateAnimationPose(SkeletonAnimationPose& outData, const glm::mat4& parentWorldTransform)
	{
		glm::mat4 worldTransform = parentWorldTransform * localTransform.Matrix();
		outData.worldTransforms.push_back(worldTransform);

		for (Bone* p_child : children)
			p_child->GenerateAnimationPose(outData, worldTransform);
	}

	void Bone::MakeCopy(Bone& outCopy)
	{
		for (Bone* p_child : children)
		{
			Bone* p_childCopy = new Bone(&outCopy);
			outCopy.children.push_back(p_childCopy);
			p_child->MakeCopy(*p_childCopy);
		}

		outCopy.localTransform = localTransform;
		outCopy.localWeightVolume = localWeightVolume;
	}
}