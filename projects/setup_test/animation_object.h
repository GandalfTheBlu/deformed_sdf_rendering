#pragma once
#include "animation.h"

struct AnimationObject
{
	size_t jointCount;
	Engine::BindPose bindPose;
	Engine::Animation animation;
	Engine::AnimationPose animationPose;
	Engine::AnimationPlayer animationPlayer;

	AnimationObject();

	void Start(float duration, bool loop);
	void Restart();
	void Update(float deltaTime);
};