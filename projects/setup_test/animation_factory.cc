#include "animation_factory.h"
#include <cassert>

AnimationObject::AnimationObject() :
	jointCount(0)
{}


void AnimationObject::Start(float duration, bool loop)
{
	animationPlayer.Start(duration, loop);
}

void AnimationObject::Update(float deltaTime)
{
	animationPlayer.Update(deltaTime, jointCount, bindPose, animation, animationPose);
}

AnimationObjectFactory::State::State() :
	stage(Stage::None),
	p_builder(nullptr)
{}


AnimationObjectFactory::Builder::Builder(AnimationObjectFactory* _p_factory) :
	p_factory(_p_factory)
{}

AnimationObjectFactory::Builder::~Builder()
{}


AnimationObjectFactory::SkeletonBuilder::SkeletonBuilder(AnimationObjectFactory* _p_factory) :
	Builder(_p_factory)
{
	p_currentJoint = &skeleton.root;
}

AnimationObjectFactory::SkeletonBuilder& 
AnimationObjectFactory::SkeletonBuilder::SetJointTransform(const Engine::Transform& transform)
{
	p_currentJoint->localTransform = transform;
	return *this;
}

AnimationObjectFactory::SkeletonBuilder& 
AnimationObjectFactory::SkeletonBuilder::SetJointWeightVolume(const Engine::JointWeightVolume& weightVolume)
{
	p_currentJoint->weightVolume = weightVolume;
	return *this;
}

AnimationObjectFactory::SkeletonBuilder& 
AnimationObjectFactory::SkeletonBuilder::AddAndGoToChild()
{
	p_currentJoint->AddChild();
	p_currentJoint = p_currentJoint->children.back();
	return *this;
}

AnimationObjectFactory::SkeletonBuilder& 
AnimationObjectFactory::SkeletonBuilder::RemoveChild(size_t index)
{
	p_currentJoint->RemoveChild(index);
	return *this;
}

AnimationObjectFactory::SkeletonBuilder& 
AnimationObjectFactory::SkeletonBuilder::GoToChild(size_t index)
{
	p_currentJoint = p_currentJoint->children[index];
	return *this;
}

AnimationObjectFactory::SkeletonBuilder& 
AnimationObjectFactory::SkeletonBuilder::GoToParent()
{
	p_currentJoint = p_currentJoint->p_parent;
	return *this;
}

size_t AnimationObjectFactory::SkeletonBuilder::GetChildCount() const
{
	return p_currentJoint->children.size();
}

bool AnimationObjectFactory::SkeletonBuilder::HasParent() const
{
	return p_currentJoint->p_parent != nullptr;
}

AnimationObjectFactory& AnimationObjectFactory::SkeletonBuilder::Complete()
{
	skeleton.BuildSkeletonAndBindPose(
		p_factory->p_state->skeleton, 
		p_factory->p_state->animationObject->bindPose
	);
	p_factory->p_state->animationObject->jointCount = skeleton.jointCount;
	p_factory->p_state->animationObject->animationPose.Allocate(skeleton.jointCount);

	p_factory->p_state->stage = AnimationObjectFactory::State::Stage::SkeletonCompleted;

	return *p_factory;
}


AnimationObjectFactory::AnimationBuilder::AnimationBuilder(AnimationObjectFactory* _p_factory) :
	Builder(_p_factory),
	currentKeyframeIndex(0)
{
	animation.InitBorderKeyframes(p_factory->p_state->skeleton);
	p_currentJoint = &animation.keyframes[0]->skeleton.root;
}

AnimationObjectFactory::AnimationBuilder& 
AnimationObjectFactory::AnimationBuilder::AddAndGoToKeyframe(float time)
{
	animation.AddKeyframe(time, currentKeyframeIndex);
	p_currentJoint = &animation.keyframes[currentKeyframeIndex]->skeleton.root;
	return *this;
}

AnimationObjectFactory::AnimationBuilder& 
AnimationObjectFactory::AnimationBuilder::RemoveKeyframe(size_t index)
{
	animation.RemoveKeyframe(index);
	return *this;
}

AnimationObjectFactory::AnimationBuilder& 
AnimationObjectFactory::AnimationBuilder::GoToKeyframe(size_t index)
{
	currentKeyframeIndex = index;
	p_currentJoint = &animation.keyframes[currentKeyframeIndex]->skeleton.root;
	return *this;
}

size_t AnimationObjectFactory::AnimationBuilder::GetKeyframeCount()
{
	return animation.keyframes.size();
}

AnimationObjectFactory::AnimationBuilder& 
AnimationObjectFactory::AnimationBuilder::SetJointTransform(const Engine::Transform& transform)
{
	p_currentJoint->localTransform = transform;
	return *this;
}

AnimationObjectFactory::AnimationBuilder& 
AnimationObjectFactory::AnimationBuilder::GoToChild(size_t index)
{
	p_currentJoint = &p_currentJoint->p_children[index];
	return *this;
}

AnimationObjectFactory::AnimationBuilder& 
AnimationObjectFactory::AnimationBuilder::GoToParent()
{
	p_currentJoint = p_currentJoint->p_parent;
	return *this;
}

size_t AnimationObjectFactory::AnimationBuilder::GetChildCount() const
{
	return p_currentJoint->childCount;
}

bool AnimationObjectFactory::AnimationBuilder::HasParent() const
{
	return p_currentJoint->p_parent != nullptr;
}

AnimationObjectFactory& AnimationObjectFactory::AnimationBuilder::Complete()
{
	animation.BuildAnimation(p_factory->p_state->animationObject->animation);

	p_factory->p_state->stage = AnimationObjectFactory::State::Stage::AnimationCompleted;

	return *p_factory;
}


AnimationObjectFactory::AnimationObjectFactory() :
	p_state(nullptr)
{}

AnimationObjectFactory::~AnimationObjectFactory()
{
	if (p_state != nullptr)
	{
		delete p_state->p_builder;
		delete p_state;
	}
}

AnimationObjectFactory::SkeletonBuilder& AnimationObjectFactory::StartBuildingSkeleton()
{
	assert(p_state == nullptr);

	p_state = new State();
	p_state->stage = State::Stage::BuildingSkeleton;
	p_state->animationObject = std::make_shared<AnimationObject>();

	SkeletonBuilder* p_builder = new SkeletonBuilder(this);
	p_state->p_builder = p_builder;

	return *p_builder;
}

AnimationObjectFactory::SkeletonBuilder& AnimationObjectFactory::GetSkeletonBuilder()
{
	assert(p_state != nullptr && p_state->stage == State::Stage::BuildingSkeleton);
	return *dynamic_cast<SkeletonBuilder*>(p_state->p_builder);
}

AnimationObjectFactory::AnimationBuilder& AnimationObjectFactory::StartAnimating()
{
	assert(p_state != nullptr && p_state->stage == State::Stage::SkeletonCompleted);
	
	delete p_state->p_builder;
	p_state->stage = State::Stage::Animating;

	AnimationBuilder* p_builder = new AnimationBuilder(this);
	p_state->p_builder = p_builder;

	return *p_builder;
}

AnimationObjectFactory::AnimationBuilder& AnimationObjectFactory::GetAnimationBuilder()
{
	assert(p_state != nullptr && p_state->stage == State::Stage::Animating);
	return *dynamic_cast<AnimationBuilder*>(p_state->p_builder);
}

std::shared_ptr<AnimationObject> AnimationObjectFactory::CompleteObject()
{
	assert(p_state != nullptr && p_state->stage == State::Stage::AnimationCompleted);

	std::shared_ptr<AnimationObject> animationObject = p_state->animationObject;

	delete p_state->p_builder;
	delete p_state;
	p_state = nullptr;

	return animationObject;
}