#include "animation.h"
#include <gtx/quaternion.hpp>
#include <cassert>

namespace Engine
{
	Bone::Bone(bool _hasParent) :
		hasParent(_hasParent),
		p_child(nullptr),
		localTransform(1.f),
		weightVolumeRadius(1.f)
	{}

	Bone::~Bone()
	{
		if(p_child != nullptr)
			delete p_child;
	}

	void Bone::AddChild()
	{
		if (p_child == nullptr)
			p_child = new Bone(true);
	}

	void Bone::RemoveChild()
	{
		if (p_child != nullptr)
		{
			delete p_child;
			p_child = nullptr;
		}
	}

	bool Bone::HasChild()
	{
		return p_child == nullptr;
	}

	Bone& Bone::GetChild()
	{
		assert(p_child != nullptr);
		return *p_child;
	}

	void Bone::GenerateBindPose(SkeletonBindPose& outData, const glm::mat4& parentWorldTransform)
	{
		glm::mat4 worldTransform = localTransform * parentWorldTransform;

		// fill in the end point of the parent's weight volume
		if (hasParent)
			outData.weightVolumes.back().endPoint = worldTransform[3];

		// ignore rest if leaf bone
		if (!HasChild())
			return;
		
		// add incomplete weight volume and let the child fill in the end position
		outData.weightVolumes.push_back(CapsulePrimitive{
			glm::vec3(worldTransform[3]),
			glm::vec3(0.f),// child fills in with its position
			weightVolumeRadius
		});

		outData.inverseWorldTransforms.push_back(glm::inverse(worldTransform));

		// proceed with the recursion
		p_child->GenerateBindPose(outData, worldTransform);
	}

	void Bone::GenerateAnimationPose(SkeletonAnimationPose& outData, const glm::mat4& parentWorldTransform)
	{
		// ignore if leaf bone
		if (!HasChild())
			return;

		glm::mat4 worldTransform = localTransform * parentWorldTransform;
		outData.worldTransforms.push_back(worldTransform);

		p_child->GenerateAnimationPose(outData, parentWorldTransform);
	}


	Skeleton::Skeleton()
	{}

	Skeleton::~Skeleton()
	{
		for (Bone* p_rootBone : rootBones)
			delete p_rootBone;
	}

	void Skeleton::AddRootBone()
	{
		rootBones.push_back(new Bone(false));
	}

	void Skeleton::RemoveRootBone(size_t boneIndex)
	{
		delete rootBones[boneIndex];
		rootBones.erase(rootBones.begin() + boneIndex);
	}

	size_t Skeleton::RootBonesCount()
	{
		return rootBones.size();
	}

	Bone& Skeleton::GetRootBone(size_t boneIndex)
	{
		return *rootBones[boneIndex];
	}

	void Skeleton::GenerateBindPose(SkeletonBindPose& outData)
	{
		for (Bone* p_rootBone : rootBones)
			p_rootBone->GenerateBindPose(outData, glm::mat4(1.f));
	}

	void Skeleton::GenerateAnimationPose(SkeletonAnimationPose& outData)
	{
		for (Bone* p_rootBone : rootBones)
			p_rootBone->GenerateAnimationPose(outData, glm::mat4(1.f));
	}


	glm::mat4 MakeTransform(const glm::vec3& position, const glm::vec3& eulerAngles, const glm::vec3& scale)
	{
		glm::mat4 T(1.f);

		T[0][0] = scale.x;
		T[1][1] = scale.y;
		T[2][2] = scale.z;

		T = glm::mat4_cast(glm::quat(eulerAngles)) * T;

		T[3] = glm::vec4(position, 1.f);

		return T;
	}
}