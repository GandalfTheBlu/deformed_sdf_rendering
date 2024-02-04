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

struct AnimationTransform
{
	glm::vec3 position;
	glm::vec3 eulerAngles;
	float scale;

	AnimationTransform();
	AnimationTransform(const glm::vec3& _position, const glm::vec3& _eulerAngles, float _scale);
};

struct BuildingJointNode
{
	bool isCurrentJoint;
	glm::vec3 jointWorldPosition;
	glm::vec3 weightWorldStartPoint;
	glm::vec3 weightWorldStartToEnd;

	BuildingJointNode();
	BuildingJointNode(
		bool _isCurrentJoint,
		const glm::vec3& _jointWorldPosition,
		const glm::vec3& _weightWorldStartPoint,
		const glm::vec3& _weightWorldStartToEnd
	);
};

struct JointNode
{
	bool isCurrentJoint;
	glm::vec3 jointWorldPosition;

	JointNode();
	JointNode(bool _isCurrentJoint, const glm::vec3& _jointWorldPosition);
};

class AnimationObjectFactory
{
public:
	enum class Stage
	{
		None,
		BuildingSkeleton,
		SkeletonCompleted,
		Animating,
		AnimationCompleted
	};

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
		AnimationBuilder& RemoveKeyframeAndGoLeft();
		AnimationBuilder& GoToKeyframe(size_t index);
		size_t GetKeyframeCount();
		size_t GetKeyframeIndex();
		bool CanKeyframeBeRemoved();

		AnimationBuilder& SetJointTransform(const AnimationTransform& transform);
		AnimationBuilder& GoToChild(size_t index);
		AnimationBuilder& GoToParent();
		AnimationTransform GetJointTransform() const;
		size_t GetChildCount() const;
		bool HasParent() const;

		// for visualization
		void GetJointNodes(float time, std::vector<JointNode>& nodes) const;
		size_t GetJointCount() const;
		const Engine::BindPose& GetBindPose() const;
		const Engine::AnimationPose& GetAnimationPose(float time) const;

		AnimationObjectFactory& Complete();
	};

	class SkeletonBuilder : public Builder
	{
	private:
		Engine::BuildingSkeleton skeleton;
		Engine::BuildingJoint* p_currentJoint;

	public:
		SkeletonBuilder(AnimationObjectFactory* _p_factory);

		SkeletonBuilder& SetJointTransform(const AnimationTransform& transform);
		SkeletonBuilder& SetJointWeightVolume(const Engine::JointWeightVolume& weightVolume);
		SkeletonBuilder& AddChild();
		SkeletonBuilder& RemoveJointAndGoToParent();
		SkeletonBuilder& GoToChild(size_t index);
		SkeletonBuilder& GoToParent();
		AnimationTransform GetJointTransform() const;
		Engine::JointWeightVolume GetJointWeightVolume() const;
		size_t GetChildCount() const;
		bool HasParent() const;

		// for visualization
		void GetBuildingJointNodes(std::vector<BuildingJointNode>& nodes) const;

		AnimationObjectFactory& Complete();
	};

private:
	struct State
	{
		Stage stage;
		Builder* p_builder;
		Engine::Skeleton skeleton;
		std::shared_ptr<AnimationObject> animationObject;

		State();
	};

	State* p_state;

public:
	AnimationObjectFactory();
	~AnimationObjectFactory();

	Stage CurrentStage() const;

	SkeletonBuilder& StartBuildingSkeleton();
	SkeletonBuilder& GetSkeletonBuilder();
	AnimationBuilder& StartAnimating();
	AnimationBuilder& GetAnimationBuilder();
	std::shared_ptr<AnimationObject> CompleteObject();
};