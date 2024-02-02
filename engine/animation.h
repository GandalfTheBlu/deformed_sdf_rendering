#pragma once
#include "transform.h"
#include <vector>

namespace Engine
{
	// used for describing the weight of a joint analytically
	struct JointWeightVolume
	{
		glm::vec3 startPoint;
		glm::vec3 startToEnd;
		float lengthSquared;
		float falloffRate;

		JointWeightVolume();
		JointWeightVolume(const glm::vec3& _startPoint, const glm::vec3& _startToEnd, float _fallofRate);
	};

	// derived data generated when posing the skeleton in a bind pose
	struct BindPose
	{
		glm::mat4* p_inverseWorldMatrices;
		JointWeightVolume* p_worldWeightVolumes;

		BindPose();
		~BindPose();

		void Init(size_t jointCount);
	};

	// derived data generated when interpolating between keyframes in an animation
	struct AnimationPose
	{
		glm::mat4* p_deformationMatrices;// = jointAnimatedWorldMatrix * jointBindInverseWorldMatrix

		AnimationPose();
		~AnimationPose();

		void Init(size_t jointCount);
	};

	class Joint
	{
	private:
		struct Skeleton* p_skeleton;
		Joint* p_parent;
		std::vector<Joint*> children;

	public:
		Transform localTransform;// relative to parent

		Joint(Skeleton* _p_skeleton, Joint* _p_parent);
		~Joint();

		bool HasParent() const;
		Joint& GetParent();
		size_t ChildCount() const;
		Joint& GetChild(size_t index);
		void AddChild();
		void RemoveChild(size_t index);
	};

	struct Skeleton
	{
		size_t jointCount;
		Joint root;

		Skeleton();
	};

	// collection of joint transforms describing an animation pose
	struct Keyframe
	{
		float timestamp;
		Transform* p_transformBuffer;

		Keyframe();
		~Keyframe();

		void Init(size_t jointCount);
	};

	class Animation
	{
	private:
		BindPose bindPose;
		size_t jointCount;
		std::vector<Keyframe> keyframes;

		size_t GetClosestLeftKeyframeIndex(float timestamp);

	public:
		Animation();
		
		void Init(Skeleton& skeleton);
		void InsertKeyframe(float timestamp);
		void RemoveKeyframe(size_t index);
		size_t KeyframeCount() const;
		Keyframe& GetKeyframe(size_t index);
		void SetKeyframe(Skeleton& skeleton, size_t keyframeIndex);
		void SetAnimationPose(AnimationPose& pose, float timestamp);
	};
}