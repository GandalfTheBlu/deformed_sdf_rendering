#include "animation_object.h"

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