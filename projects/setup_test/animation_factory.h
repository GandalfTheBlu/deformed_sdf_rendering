#pragma once
#include "animation.h"
#include <memory>

struct AnimationObject
{
	size_t jointCount;
	Engine::BindPose bindPose;
	Engine::Animation animation;
	Engine::AnimationPose animationPose;
	Engine::AnimationPlayer animationPlayer;

	AnimationObject();

	void Start(float duration, bool loop);
	void Update(float deltaTime);
};

class AnimationObjectFactory
{
public:
	// builder interfaces for factory
	class Builder
	{
	protected:
		AnimationObjectFactory* p_factory;

	public:
		Builder(AnimationObjectFactory* _p_factory);
		virtual ~Builder();
	};

	class AnimationBuilder : public Builder
	{
	private:
		Engine::BuildingAnimation animation;
		size_t currentKeyframeIndex;
		Engine::Joint* p_currentJoint;

	public:
		AnimationBuilder(AnimationObjectFactory* _p_factory);

		AnimationBuilder& AddAndGoToKeyframe(float time);
		AnimationBuilder& RemoveKeyframe(size_t index);
		AnimationBuilder& GoToKeyframe(size_t index);
		size_t GetKeyframeCount();
		
		AnimationBuilder& SetJointTransform(const Engine::Transform& transform);
		AnimationBuilder& GoToChild(size_t index);
		AnimationBuilder& GoToParent();
		size_t GetChildCount() const;
		bool HasParent() const;

		AnimationObjectFactory& Complete();
	};

	class SkeletonBuilder : public Builder
	{
	private:
		Engine::BuildingSkeleton skeleton;
		Engine::BuildingJoint* p_currentJoint;

	public:
		SkeletonBuilder(AnimationObjectFactory* _p_factory);

		SkeletonBuilder& SetJointTransform(const Engine::Transform& transform);
		SkeletonBuilder& SetJointWeightVolume(const Engine::JointWeightVolume& weightVolume);
		SkeletonBuilder& AddAndGoToChild();
		SkeletonBuilder& RemoveChild(size_t index);
		SkeletonBuilder& GoToChild(size_t index);
		SkeletonBuilder& GoToParent();
		size_t GetChildCount() const;
		bool HasParent() const;

		AnimationObjectFactory& Complete();
	};

private:
	struct State
	{
		enum class Stage
		{
			None,
			BuildingSkeleton,
			SkeletonCompleted,
			Animating,
			AnimationCompleted
		} stage;
		Builder* p_builder;
		Engine::Skeleton skeleton;
		std::shared_ptr<AnimationObject> animationObject;

		State();
	};

	State* p_state;

public:
	AnimationObjectFactory();
	~AnimationObjectFactory();

	SkeletonBuilder& StartBuildingSkeleton();
	SkeletonBuilder& GetSkeletonBuilder();
	AnimationBuilder& StartAnimating();
	AnimationBuilder& GetAnimationBuilder();
	std::shared_ptr<AnimationObject> CompleteObject();
};