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

		CapsulePrimitive();
		CapsulePrimitive(const glm::vec3& _startPoint, const glm::vec3& _endPoint, float _radius);
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

	struct Transform
	{
		glm::vec3 position;
		glm::vec3 eulerAngles;
		glm::vec3 scale;

		Transform();
		Transform(const glm::vec3& _position, const glm::vec3& _eulerAngles, const glm::vec3& _scale);

		glm::mat4 Matrix() const;
	};

	class Bone
	{
	private:
		std::vector<Bone*> children;

	public:
		Bone* p_parent;
		Transform localTransform;// relative to parent transform
		CapsulePrimitive localWeightVolume;// defined in local space

		Bone();
		Bone(Bone* _p_parent);
		~Bone();

		void AddChild();
		void RemoveChild(size_t childIndex);
		size_t ChildrenCount();
		Bone& GetChild(size_t childIndex);

		void GenerateBindPose(SkeletonBindPose& outData, const glm::mat4& parentWorldTransform);
		void GenerateAnimationPose(SkeletonAnimationPose& outData, const glm::mat4& parentWorldTransform);

		void MakeCopy(Bone& outCopy);
	};
}