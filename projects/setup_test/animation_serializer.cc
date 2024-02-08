#include "animation_serializer.h"

void AppendAnimationObjectToBuffer(const AnimationObject& object, std::string& buffer)
{
	AppendData<size_t>(object.jointCount, buffer);
	AppendData<size_t>(object.animation.keyframeCount, buffer);
	AppendData<float>(object.animationPlayer.duration, buffer);
	AppendData<bool>(object.animationPlayer.loop, buffer);

	for (size_t i = 0; i < object.jointCount; i++)
		AppendData<glm::mat4>(object.bindPose.p_inverseWorldMatrices[i], buffer);

	for (size_t i = 0; i < object.jointCount; i++)
		AppendData<Engine::JointWeightVolume>(object.bindPose.p_worldWeightVolumes[i], buffer);

	for (size_t i = 0; i < object.animation.keyframeCount; i++)
	{
		AppendData<float>(object.animation.p_keyframes[i].timestamp, buffer);

		for (size_t j = 0; j < object.jointCount; j++)
			AppendData<Engine::Transform>(object.animation.p_keyframes[i].p_transformBuffer[j], buffer);
	}
}

void ReadAnimationObjectFromBuffer(const std::string& buffer, size_t& inoutBufferIndex, AnimationObject& outObject)
{
	ReadData<size_t>(buffer, inoutBufferIndex, outObject.jointCount);
	ReadData<size_t>(buffer, inoutBufferIndex, outObject.animation.keyframeCount);
	ReadData<float>(buffer, inoutBufferIndex, outObject.animationPlayer.duration);
	ReadData<bool>(buffer, inoutBufferIndex, outObject.animationPlayer.loop);

	outObject.animation.Allocate(outObject.animation.keyframeCount);
	outObject.bindPose.Allocate(outObject.jointCount);
	outObject.animationPose.Allocate(outObject.jointCount);

	for (size_t i = 0; i < outObject.jointCount; i++)
		ReadData<glm::mat4>(buffer, inoutBufferIndex, outObject.bindPose.p_inverseWorldMatrices[i]);

	for (size_t i = 0; i < outObject.jointCount; i++)
		ReadData<Engine::JointWeightVolume>(buffer, inoutBufferIndex, outObject.bindPose.p_worldWeightVolumes[i]);

	for (size_t i = 0; i < outObject.animation.keyframeCount; i++)
	{
		outObject.animation.p_keyframes[i].Allocate(outObject.jointCount);
		ReadData<float>(buffer, inoutBufferIndex, outObject.animation.p_keyframes[i].timestamp);

		for (size_t j = 0; j < outObject.jointCount; j++)
			ReadData<Engine::Transform>(buffer, inoutBufferIndex, outObject.animation.p_keyframes[i].p_transformBuffer[j]);
	}
}