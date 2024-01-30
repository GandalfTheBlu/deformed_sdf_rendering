#pragma once
#include <matrix.hpp>
#include <vector>

namespace Engine
{
	struct CapsulePrimitive
	{
		glm::vec3 startPoint;
		glm::vec3 endPoint;
		float radius;
	};

	struct SkeletonBindPose
	{
		// each bone is represented by its inverse world transform and
		// its weight volume (within which its weight is > 0)
		std::vector<glm::mat4> inverseWorldTransforms;
		std::vector<CapsulePrimitive> weightVolumes;
	};

	struct SkeletonAnimationPose
	{
		// each bone is represented by its deformed world transform
		std::vector<glm::mat4> worldTransforms;
	};

	class Bone
	{
	private:
		Bone* p_child;

	public:
		bool hasParent;
		glm::mat4 localTransform;// relative to parent transform
		float weightVolumeRadius;

		Bone() = delete;
		Bone(bool _hasParent);
		~Bone();

		void AddChild();
		void RemoveChild();
		bool HasChild();
		Bone& GetChild();

		void GenerateBindPose(SkeletonBindPose& outData, const glm::mat4& parentWorldTransform);
		void GenerateAnimationPose(SkeletonAnimationPose& outData, const glm::mat4& parentWorldTransform);
	};

	class Skeleton
	{
	private:
		std::vector<Bone*> rootBones;

	public:
		Skeleton();
		~Skeleton();

		void AddRootBone();
		void RemoveRootBone(size_t boneIndex);
		size_t RootBonesCount();
		Bone& GetRootBone(size_t boneIndex);

		void GenerateBindPose(SkeletonBindPose& outData);
		void GenerateAnimationPose(SkeletonAnimationPose& outData);
	};

	glm::mat4 MakeTransform(const glm::vec3& position, const glm::vec3& eulerAngles, const glm::vec3& scale);
}