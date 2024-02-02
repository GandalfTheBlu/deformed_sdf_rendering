#include "animation.h"
#include <gtx/quaternion.hpp>

namespace Engine
{
	JointWeightVolume::JointWeightVolume() :
		startPoint(0.f),
		startToEnd(0.f),
		lengthSquared(0.f),
		falloffRate(0.f)
	{}

	JointWeightVolume::JointWeightVolume(const glm::vec3& _startPoint, const glm::vec3& _startToEnd, float _fallofRate) :
		startPoint(_startPoint),
		startToEnd(_startToEnd),
		lengthSquared(glm::dot(_startToEnd, _startToEnd)),
		falloffRate(_fallofRate)
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

	void BindPose::Init(size_t jointCount)
	{
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

	void AnimationPose::Init(size_t jointCount)
	{
		p_deformationMatrices = new glm::mat4[jointCount]();
	}


	Joint::Joint(Skeleton* _p_skeleton, Joint* _p_parent) :
		p_skeleton(_p_skeleton),
		p_parent(_p_parent)
	{}

	Joint::~Joint()
	{
		for (Joint* p_child : children)
			delete p_child;
	}

	bool Joint::HasParent() const
	{
		return p_parent != nullptr;
	}

	Joint& Joint::GetParent()
	{
		assert(p_parent != nullptr);
		return *p_parent;
	}

	size_t Joint::ChildCount() const
	{
		return children.size();
	}

	Joint& Joint::GetChild(size_t index)
	{
		return *children[index];
	}

	void Joint::AddChild()
	{
		children.push_back(new Joint(p_skeleton, this));
		p_skeleton->jointCount++;
	}

	void Joint::RemoveChild(size_t index)
	{
		delete children[index];
		children.erase(children.begin() + index);
		p_skeleton->jointCount--;
	}


	Keyframe::Keyframe() :
		p_transformBuffer(nullptr)
	{}

	Keyframe::~Keyframe()
	{
		delete[] p_transformBuffer;
	}

	void Keyframe::Init(size_t jointCount)
	{
		p_transformBuffer = new Transform[jointCount]();
	}


	Skeleton::Skeleton() :
		jointCount(0),
		root(this, nullptr)
	{}


	Animation::Animation() :
		jointCount(0)
	{}

	void Animation::Init(Skeleton& skeleton)
	{
		InsertKeyframe(0.f);
		InsertKeyframe(1.f);
		
	}

	void Animation::InsertKeyframe(float timestamp)
	{
		assert(timestamp >= 0.f && timestamp <= 1.f);

		if (keyframes.size() == 0)
		{
			keyframes.emplace_back();
			keyframes.back().Init(jointCount);
			return;
		}

		for (size_t i = 1; i < keyframes.size(); i++)
		{
			if (keyframes[i].timestamp == timestamp)
				break;// avoid duplicate
			
			if (keyframes[i].timestamp > timestamp)
			{
				keyframes.emplace(keyframes.begin() + i);
				keyframes[i].Init(jointCount);
				break;
			}
		}
	}

	size_t Animation::GetClosestLeftKeyframeIndex(float timestamp)
	{
		assert(timestamp >= 0.f && timestamp <= 1.f && keyframes.size() > 0);

		for (size_t i = 1; i < keyframes.size(); i++)
		{
			if (keyframes[i].timestamp > timestamp)
				return i - 1;
		}

		return keyframes.size() - 1;
	}

	void Animation::RemoveKeyframe(size_t index)
	{
		keyframes.erase(keyframes.begin() + index);
	}

	size_t Animation::KeyframeCount() const
	{
		return keyframes.size();
	}

	Keyframe& Animation::GetKeyframe(size_t index)
	{
		return keyframes[index];
	}

	struct KeyframeBuilder
	{
		Keyframe* p_keyframe;
		size_t transformCount;
	};

	void BuildKeyframe(Joint& startJoint, const Transform& parentWorldTransform, KeyframeBuilder& builder)
	{
		Transform worldTransform = Multiply(parentWorldTransform, startJoint.localTransform);
		builder.p_keyframe->p_transformBuffer[builder.transformCount++] = worldTransform;

		for (size_t i = 0; i < startJoint.ChildCount(); i++)
			BuildKeyframe(startJoint.GetChild(i), worldTransform, builder);
	}

	void Animation::SetKeyframe(Skeleton& skeleton, size_t keyframeIndex)
	{
		KeyframeBuilder builder{ &GetKeyframe(keyframeIndex), 0 };
		BuildKeyframe(skeleton.root, Transform(), builder);
	}

	void Animation::SetAnimationPose(AnimationPose& pose, float timestamp)
	{
		size_t leftIndex = GetClosestLeftKeyframeIndex(timestamp);
		size_t rightIndex = leftIndex + 1;

		if (leftIndex == keyframes.size() - 1)
		{
			rightIndex = leftIndex;
			leftIndex = 0;
		}

		Keyframe& leftKeyframe = GetKeyframe(leftIndex);
		Keyframe& rightKeyframe = GetKeyframe(rightIndex);
		float keyframeTimeDiff = glm::abs(rightKeyframe.timestamp - leftKeyframe.timestamp);
		float alpha = (timestamp - glm::min(leftKeyframe.timestamp, rightKeyframe.timestamp)) / keyframeTimeDiff;

		for (size_t i = 0; i < jointCount; i++)
		{
			pose.p_deformationMatrices[i] = Lerp(
				leftKeyframe.p_transformBuffer[i],
				rightKeyframe.p_transformBuffer[i],
				alpha
			).Matrix() * bindPose.p_inverseWorldMatrices[i];
		}
	}
}