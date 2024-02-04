#include "animation.h"
#include <gtx/quaternion.hpp>
#include <cassert>

namespace Engine
{
	JointWeightVolume::JointWeightVolume() :
		startPoint(0.f),
		startToEnd(0.f, 1.f, 0.f),
		falloffRate(10.f)
	{}


	BindPose::BindPose() :
		p_inverseWorldMatrices(nullptr),
		p_worldWeightVolumes(nullptr)
	{}

	BindPose::~BindPose()
	{
		delete[] p_inverseWorldMatrices;
		delete[] p_worldWeightVolumes;
	}

	void BindPose::Allocate(size_t jointCount)
	{
		assert(p_inverseWorldMatrices == nullptr && p_worldWeightVolumes == nullptr);
		p_inverseWorldMatrices = new glm::mat4[jointCount]();
		p_worldWeightVolumes = new JointWeightVolume[jointCount]();
	}


	AnimationPose::AnimationPose() :
		p_deformationMatrices(nullptr)
	{}

	AnimationPose::~AnimationPose()
	{
		delete[] p_deformationMatrices;
	}

	void AnimationPose::Allocate(size_t jointCount)
	{
		assert(p_deformationMatrices == nullptr);
		p_deformationMatrices = new glm::mat4[jointCount]();
	}


	Keyframe::Keyframe() :
		timestamp(0.f),
		p_transformBuffer(nullptr)
	{}

	Keyframe::~Keyframe()
	{
		delete[] p_transformBuffer;
	}

	void Keyframe::Allocate(size_t jointCount)
	{
		assert(p_transformBuffer == nullptr);
		p_transformBuffer = new Transform[jointCount]();
	}

	Animation::Animation() :
		keyframeCount(0),
		p_keyframes(nullptr)
	{}

	Animation::~Animation()
	{
		delete[] p_keyframes;
	}

	void Animation::Allocate(size_t _keyframeCount)
	{
		assert(p_keyframes == nullptr);
		keyframeCount = _keyframeCount;
		p_keyframes = new Keyframe[keyframeCount]();
	}


	AnimationPlayer::AnimationPlayer() :
		currentKeyframeIndex(0),
		currentTime(0.f),
		duration(0.f),
		loop(false)
	{}

	void AnimationPlayer::Start(float _duration, bool _loop)
	{
		currentKeyframeIndex = 0;
		currentTime = 0.f;
		duration = _duration;
		loop = _loop;
	}

	void AnimationPlayer::Update(
		float deltaTime,
		size_t jointCount,
		const BindPose& bindPose,
		const Animation& animation,
		AnimationPose& animationPose
	)
	{
		Keyframe& leftKeyframe = animation.p_keyframes[currentKeyframeIndex];
		Keyframe& rightKeyframe = animation.p_keyframes[currentKeyframeIndex + 1];
		float normalizedTime = currentTime / duration;
		float alpha = (normalizedTime - leftKeyframe.timestamp) / 
			(rightKeyframe.timestamp - leftKeyframe.timestamp);

		for (size_t i = 0; i < jointCount; i++)
		{
			animationPose.p_deformationMatrices[i] =
				Lerp(leftKeyframe.p_transformBuffer[i], rightKeyframe.p_transformBuffer[i], alpha).Matrix() *
				bindPose.p_inverseWorldMatrices[i];
		}

		currentTime += deltaTime;
		if (currentTime / duration > rightKeyframe.timestamp)
		{
			currentKeyframeIndex++;

			if (currentKeyframeIndex + 1 == animation.keyframeCount)
			{
				if (loop)
				{
					currentKeyframeIndex = 0;
					currentTime = 0.f;
				}
				else
				{
					currentKeyframeIndex--;
					currentTime = duration;
				}
			}
		}
	}

	bool AnimationPlayer::IsDone() const
	{
		return !loop && currentTime >= duration;
	}


	Joint::Joint() :
		p_parent(nullptr),
		childCount(0),
		p_children(nullptr),
		eulerAngles(0.f)
	{}

	Joint::~Joint()
	{
		delete[] p_children;
	}

	void Joint::Allocate(size_t _childrenCount)
	{
		childCount = _childrenCount;

		if(childCount > 0)
			p_children = new Joint[childCount]();
	}


	Skeleton::Skeleton() :
		jointCount(1)
	{}

	void CopyJoints(const Joint& startJoint, Joint& jointCopy)
	{
		jointCopy.localTransform = startJoint.localTransform;
		jointCopy.Allocate(startJoint.childCount);
		
		for (size_t i = 0; i < startJoint.childCount; i++)
		{
			jointCopy.p_children[i].p_parent = &jointCopy;
			CopyJoints(startJoint.p_children[i], jointCopy.p_children[i]);
		}
	}

	void Skeleton::MakeCopy(Skeleton& copySkeleton) const
	{
		copySkeleton.jointCount = jointCount;
		CopyJoints(root, copySkeleton.root);
	}


	BuildingJoint::BuildingJoint(BuildingSkeleton* _p_skeleton, BuildingJoint* _p_parent) :
		p_skeleton(_p_skeleton),
		p_parent(_p_parent),
		eulerAngles(0.f)
	{}

	BuildingJoint::~BuildingJoint()
	{
		for (BuildingJoint* p_child : children)
			delete p_child;
	}

	void BuildingJoint::AddChild()
	{
		children.push_back(new BuildingJoint(p_skeleton, this));
		p_skeleton->jointCount++;
	}

	void BuildingJoint::RemoveChild(size_t index)
	{
		delete children[index];
		children.erase(children.begin() + index);
		p_skeleton->jointCount--;
	}


	BuildingSkeleton::BuildingSkeleton() :
		jointCount(1),
		root(this, nullptr)
	{}

	void BuildJoints(const BuildingJoint& startBuildingJoint, Joint* p_parent, Joint& joint)
	{
		joint.p_parent = p_parent;
		joint.localTransform = startBuildingJoint.localTransform;
		joint.Allocate(startBuildingJoint.children.size());

		for (size_t i = 0; i < startBuildingJoint.children.size(); i++)
			BuildJoints(*startBuildingJoint.children[i], &joint, joint.p_children[i]);
	}

	void BuildBindPoses(
		const BuildingJoint& startBuildingJoint,
		const glm::mat4& parentWorldTransform,
		BindPose& bindPose,
		size_t& inoutIndex)
	{
		glm::mat4 worldTransform = parentWorldTransform * startBuildingJoint.localTransform.Matrix();
		bindPose.p_inverseWorldMatrices[inoutIndex] = glm::inverse(worldTransform);
		JointWeightVolume& weightVolume = bindPose.p_worldWeightVolumes[inoutIndex];
		weightVolume.startPoint = glm::vec3(worldTransform * glm::vec4(startBuildingJoint.weightVolume.startPoint, 1.f));
		weightVolume.startToEnd = glm::vec3(parentWorldTransform * glm::vec4(startBuildingJoint.weightVolume.startToEnd, 0.f));
		weightVolume.falloffRate = startBuildingJoint.weightVolume.falloffRate;

		for (size_t i = 0; i < startBuildingJoint.children.size(); i++)
			BuildBindPoses(*startBuildingJoint.children[i], worldTransform, bindPose, ++inoutIndex);
	}

	void BuildingSkeleton::BuildSkeletonAndBindPose(Skeleton& skeleton, BindPose& bindPose) const
	{
		skeleton.jointCount = jointCount;
		BuildJoints(root, nullptr, skeleton.root);

		bindPose.Allocate(jointCount);
		size_t index = 0;
		BuildBindPoses(root, glm::mat4(1.f), bindPose, index);
	}


	BuildingKeyframe::BuildingKeyframe(float _timestamp) :
		timestamp(_timestamp)
	{}


	BuildingAnimation::BuildingAnimation()
	{}

	BuildingAnimation::~BuildingAnimation()
	{
		for (BuildingKeyframe* p_keyframe : keyframes)
			delete p_keyframe;
	}

	void BuildingAnimation::InitBorderKeyframes(const Skeleton& skeleton)
	{
		keyframes.push_back(new BuildingKeyframe(0.f));
		keyframes.push_back(new BuildingKeyframe(1.f));

		skeleton.MakeCopy(keyframes[0]->skeleton);
		skeleton.MakeCopy(keyframes[1]->skeleton);
	}

	void InterpolateJoints(const Joint& startLeftJoint, const Joint& startRightJoint, Joint& joint, float alpha)
	{
		joint.localTransform = Lerp(startLeftJoint.localTransform, startRightJoint.localTransform, alpha);

		for (size_t i = 0; i < joint.childCount; i++)
			InterpolateJoints(joint.p_children[i], startLeftJoint.p_children[i], startRightJoint.p_children[i], alpha);
	}

	void BuildingAnimation::AddKeyframe(float timestamp, size_t& outIndex)
	{
		// check if initialized
		assert(keyframes.size() > 1);

		for (size_t i = 1; i < keyframes.size(); i++)
		{
			// avoid duplicates
			if (keyframes[i]->timestamp == timestamp)
			{
				outIndex = i;
				break;
			}

			if (keyframes[i]->timestamp > timestamp)
			{
				// interpolate new skeleton between surrounding poses
				BuildingKeyframe* p_newKeyframe = new BuildingKeyframe(timestamp);
				keyframes[i - 1]->skeleton.MakeCopy(p_newKeyframe->skeleton);
				float alpha = (timestamp - keyframes[i - 1]->timestamp) /
					(keyframes[i]->timestamp - keyframes[i - 1]->timestamp);

				InterpolateJoints(
					keyframes[i - 1]->skeleton.root,
					keyframes[i]->skeleton.root,
					p_newKeyframe->skeleton.root,
					alpha
				);

				//GetSkeletonPose(timestamp, p_newKeyframe->skeleton);

				keyframes.insert(keyframes.begin() + i, p_newKeyframe);
				outIndex = i;

				break;
			}
		}
	}

	void BuildingAnimation::RemoveKeyframe(size_t index)
	{
		assert(index > 0 && index < keyframes.size() - 1);

		delete keyframes[index];
		keyframes.erase(keyframes.begin() + index);
	}

	void InterpolateAnimationPoses(
		const Joint& startLeftJoint, 
		const Joint& startRightJoint, 
		const Transform& leftParentTransform,
		const Transform& rightParentTransform,
		const BindPose& bindPose, 
		AnimationPose& animationPose,
		size_t& inoutIndex,
		float alpha)
	{
		Transform leftWorldTransform = Multiply(leftParentTransform, startLeftJoint.localTransform);
		Transform rightWorldTransform = Multiply(rightParentTransform, startRightJoint.localTransform);

		animationPose.p_deformationMatrices[inoutIndex] = Lerp(
			leftWorldTransform,
			rightWorldTransform,
			alpha
		).Matrix() * bindPose.p_inverseWorldMatrices[inoutIndex];

		for (size_t i = 0; i < startLeftJoint.childCount; i++)
		{
			InterpolateAnimationPoses(
				startLeftJoint.p_children[i],
				startRightJoint.p_children[i],
				leftWorldTransform,
				rightWorldTransform,
				bindPose,
				animationPose,
				++inoutIndex,
				alpha
			);
		}
	}

	void BuildingAnimation::GetAnimationPose(
		float time, 
		const BindPose& bindPose, 
		AnimationPose& animationPose
	) const
	{
		for (size_t i = 1; i < keyframes.size(); i++)
		{
			if (keyframes[i]->timestamp >= time)
			{
				size_t index = 0;
				float alpha = (time - keyframes[i - 1]->timestamp) /
					(keyframes[i]->timestamp - keyframes[i - 1]->timestamp);

				InterpolateAnimationPoses(
					keyframes[i - 1]->skeleton.root,
					keyframes[i]->skeleton.root,
					Transform(),
					Transform(),
					bindPose,
					animationPose,
					index,
					alpha	
				);
				break;
			}
		}
	}

	void BuildKeyframe(const Joint& startJoint, const Transform& parentWorldTransform, Keyframe& keyframe, size_t& inoutKeyframeIndex)
	{
		Transform worldTransform = Multiply(parentWorldTransform, startJoint.localTransform);
		keyframe.p_transformBuffer[inoutKeyframeIndex] = worldTransform;

		for (size_t i = 0; i < startJoint.childCount; i++)
		{
			BuildKeyframe(startJoint.p_children[i], worldTransform, keyframe, ++inoutKeyframeIndex);
		}
	}

	void BuildingAnimation::BuildAnimation(Animation& animation) const
	{
		animation.Allocate(keyframes.size());

		for (size_t i = 0; i < keyframes.size(); i++)
		{
			animation.p_keyframes[i].timestamp = keyframes[i]->timestamp;
			animation.p_keyframes[i].Allocate(keyframes[i]->skeleton.jointCount);
			size_t index = 0;
			BuildKeyframe(keyframes[i]->skeleton.root, Transform(), animation.p_keyframes[i], index);
		}
	}
}