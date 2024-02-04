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
		float falloffRate;

		JointWeightVolume();
	};

	// derived data generated when posing the skeleton in a bind pose
	struct BindPose
	{
		glm::mat4* p_inverseWorldMatrices;
		JointWeightVolume* p_worldWeightVolumes;

		BindPose();
		~BindPose();

		void Allocate(size_t jointCount);
	};

	// derived data generated when interpolating between keyframes in an animation
	struct AnimationPose
	{
		glm::mat4* p_deformationMatrices;// = jointAnimatedWorldMatrix * jointBindInverseWorldMatrix

		AnimationPose();
		~AnimationPose();

		void Allocate(size_t jointCount);
	};

	// collection of joint world transforms describing an animation pose
	struct Keyframe
	{
		float timestamp;
		Transform* p_transformBuffer;

		Keyframe();
		~Keyframe();

		void Allocate(size_t jointCount);
	};

	// collection of keyframes used for interpolating an animation pose over time
	struct Animation
	{
		size_t keyframeCount;
		Keyframe* p_keyframes;

		Animation();
		~Animation();

		void Allocate(size_t _keyframeCount);
	};

	// manages animation duration, looping, keyframe selection and interpolation
	struct AnimationPlayer
	{
		size_t currentKeyframeIndex;
		float currentTime;
		float duration;
		bool loop;

		AnimationPlayer();

		void Start(float _duration, bool _loop);
		void Update(
			float deltaTime,
			size_t jointCount, 
			const BindPose& bindPose, 
			const Animation& animation, 
			AnimationPose& animationPose
		);
		bool IsDone() const;
	};

	// streamlined representation of joint used when generating keyframes
	struct Joint
	{
		Joint* p_parent;
		size_t childCount;
		Joint* p_children;
		Transform localTransform;
		glm::vec3 eulerAngles;

		Joint();
		~Joint();

		void Allocate(size_t _childrenCount);
	};

	// streamlined representation of skeleton used when generating keyframes
	struct Skeleton
	{
		size_t jointCount;
		Joint root;

		Skeleton();

		void MakeCopy(Skeleton& copySkeleton) const;
	};

	struct BuildingSkeleton;

	// joint representation used when building a skeleton and a bind pose
	struct BuildingJoint
	{
		BuildingSkeleton* p_skeleton;
		BuildingJoint* p_parent;
		std::vector<BuildingJoint*> children;
		Transform localTransform;
		glm::vec3 eulerAngles;
		JointWeightVolume weightVolume;

		BuildingJoint(BuildingSkeleton* _p_skeleton, BuildingJoint* _p_parent);
		~BuildingJoint();

		void AddChild();
		void RemoveChild(size_t index);
	};

	// used for building a streamlined skeleton and a bind pose
	struct BuildingSkeleton
	{
		size_t jointCount;
		BuildingJoint root;

		BuildingSkeleton();

		void BuildSkeletonAndBindPose(Skeleton& skeleton, BindPose& bindPose) const;
	};
	
	// stores skeleton pose used when building keyframes
	struct BuildingKeyframe
	{
		float timestamp;
		Skeleton skeleton;

		BuildingKeyframe(float _timestamp);
	};

	// stores building keyframes used for building a streamlined animation
	struct BuildingAnimation
	{
		std::vector<BuildingKeyframe*> keyframes;

		BuildingAnimation();
		~BuildingAnimation();

		void InitBorderKeyframes(const Skeleton& skeleton);
		// inserts a keyframe at the correct position and 
		// iterpolates its skeleton between the surrounding poses
		void AddKeyframe(float timestamp, size_t& outIndex);
		void RemoveKeyframe(size_t index);

		// used for visualization while building
		void GetAnimationPose(float time, const BindPose& bindPose, AnimationPose& animationPose) const;

		void BuildAnimation(Animation& animation) const;
	};
}