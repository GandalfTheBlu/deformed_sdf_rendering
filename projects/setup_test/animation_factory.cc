#include "animation_factory.h"
#include <cassert>


AnimationTransform::AnimationTransform() :
	position(0.f),
	eulerAngles(0.f),
	scale(1.f)
{}

AnimationTransform::AnimationTransform(const glm::vec3& _position, const glm::vec3& _eulerAngles, float _scale) :
	position(_position),
	eulerAngles(_eulerAngles),
	scale(_scale)
{}


BuildingJointNode::BuildingJointNode() :
	isCurrentJoint(false),
	jointWorldPosition(0.f),
	weightWorldStartPoint(0.f),
	weightWorldStartToEnd(0.f)
{}

BuildingJointNode::BuildingJointNode(
	bool _isCurrentJoint,
	const glm::vec3& _jointWorldPosition,
	const glm::vec3& _weightWorldStartPoint,
	const glm::vec3& _weightWorldStartToEnd
) :
	isCurrentJoint(_isCurrentJoint),
	jointWorldPosition(_jointWorldPosition),
	weightWorldStartPoint(_weightWorldStartPoint),
	weightWorldStartToEnd(_weightWorldStartToEnd)
{}


JointNode::JointNode() :
	isCurrentJoint(false),
	jointWorldPosition(0.f)
{}

JointNode::JointNode(bool _isCurrentJoint, const glm::vec3& _jointWorldPosition) :
	isCurrentJoint(_isCurrentJoint),
	jointWorldPosition(_jointWorldPosition)
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
AnimationObjectFactory::SkeletonBuilder::SetJointTransform(const AnimationTransform& transform)
{
	p_currentJoint->localTransform.position = transform.position;
	p_currentJoint->localTransform.rotation = transform.eulerAngles;
	p_currentJoint->localTransform.scale = transform.scale;
	p_currentJoint->eulerAngles = transform.eulerAngles;
	return *this;
}

AnimationObjectFactory::SkeletonBuilder& 
AnimationObjectFactory::SkeletonBuilder::SetJointWeightVolume(const Engine::JointWeightVolume& weightVolume)
{
	p_currentJoint->weightVolume = weightVolume;
	return *this;
}

AnimationObjectFactory::SkeletonBuilder& 
AnimationObjectFactory::SkeletonBuilder::AddChild()
{
	p_currentJoint->AddChild();
	p_currentJoint->children.back()->localTransform.position = 
		p_currentJoint->weightVolume.startPoint + p_currentJoint->weightVolume.startToEnd;

	return *this;
}

AnimationObjectFactory::SkeletonBuilder& 
AnimationObjectFactory::SkeletonBuilder::RemoveJointAndGoToParent()
{
	assert(p_currentJoint->p_parent != nullptr);

	Engine::BuildingJoint* p_parent = p_currentJoint->p_parent;

	for (size_t i = 0; i < p_parent->children.size(); i++)
	{
		if (p_parent->children[i] == p_currentJoint)
		{
			p_parent->RemoveChild(i);
			p_currentJoint = p_parent;
			break;
		}
	}
	
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

AnimationTransform AnimationObjectFactory::SkeletonBuilder::GetJointTransform() const
{
	return AnimationTransform(
		p_currentJoint->localTransform.position,
		p_currentJoint->eulerAngles,
		p_currentJoint->localTransform.scale
	);
}

Engine::JointWeightVolume AnimationObjectFactory::SkeletonBuilder::GetJointWeightVolume() const
{
	return p_currentJoint->weightVolume;
}

size_t AnimationObjectFactory::SkeletonBuilder::GetChildCount() const
{
	return p_currentJoint->children.size();
}

bool AnimationObjectFactory::SkeletonBuilder::HasParent() const
{
	return p_currentJoint->p_parent != nullptr;
}

void BuildBuildingJointNodes(
	const Engine::BuildingJoint& startBuildingJoint,
	const glm::mat4& parentWorldTransform,
	const Engine::BuildingJoint* p_currentJoint,
	std::vector<BuildingJointNode>& nodes
)
{
	glm::mat4 worldTransform = parentWorldTransform * startBuildingJoint.localTransform.Matrix();
	bool isCurrentJoint = &startBuildingJoint == p_currentJoint;

	nodes.emplace_back(
		isCurrentJoint,
		glm::vec3(worldTransform[3]),
		glm::vec3(worldTransform * glm::vec4(startBuildingJoint.weightVolume.startPoint, 1.f)),
		glm::vec3(parentWorldTransform * glm::vec4(startBuildingJoint.weightVolume.startToEnd, 0.f))
	);

	for (Engine::BuildingJoint* p_child : startBuildingJoint.children)
		BuildBuildingJointNodes(*p_child, worldTransform, p_currentJoint, nodes);
}

void AnimationObjectFactory::SkeletonBuilder::GetBuildingJointNodes(
	std::vector<BuildingJointNode>& nodes
) const
{
	nodes.clear();
	BuildBuildingJointNodes(skeleton.root, glm::mat4(1.f), p_currentJoint, nodes);
}

void BuildWeightVolumes(
	const Engine::BuildingJoint& startBuildingJoint,
	const glm::mat4& parentWorldTransform,
	std::vector<Engine::JointWeightVolume>& weightVolumes
)
{
	glm::mat4 worldTransform = parentWorldTransform * startBuildingJoint.localTransform.Matrix();
	weightVolumes.emplace_back();
	auto& weightVolume = weightVolumes.back();
	weightVolume.startPoint = glm::vec3(worldTransform * glm::vec4(startBuildingJoint.weightVolume.startPoint, 1.f));
	weightVolume.startToEnd = glm::vec3(parentWorldTransform * glm::vec4(startBuildingJoint.weightVolume.startToEnd, 0.f));
	weightVolume.falloffRate = startBuildingJoint.weightVolume.falloffRate;

	for (size_t i = 0; i < startBuildingJoint.children.size(); i++)
		BuildWeightVolumes(*startBuildingJoint.children[i], worldTransform, weightVolumes);
}

void AnimationObjectFactory::SkeletonBuilder::GetWorldJointWeightVolumes(
	std::vector<Engine::JointWeightVolume>& weightVolumes
) const
{
	weightVolumes.clear();
	BuildWeightVolumes(skeleton.root, glm::mat4(1.f), weightVolumes);
}

AnimationObjectFactory& AnimationObjectFactory::SkeletonBuilder::Complete()
{
	skeleton.BuildSkeletonAndBindPose(
		p_factory->p_state->skeleton, 
		p_factory->p_state->animationObject->bindPose
	);
	p_factory->p_state->animationObject->jointCount = skeleton.jointCount;
	p_factory->p_state->animationObject->animationPose.Allocate(skeleton.jointCount);

	p_factory->p_state->stage = AnimationObjectFactory::Stage::SkeletonCompleted;

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
AnimationObjectFactory::AnimationBuilder::RemoveKeyframeAndGoLeft()
{
	assert(currentKeyframeIndex > 0 && currentKeyframeIndex + 1 < animation.keyframes.size());

	animation.RemoveKeyframe(currentKeyframeIndex);
	currentKeyframeIndex--;
	p_currentJoint = &animation.keyframes[currentKeyframeIndex]->skeleton.root;

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

size_t AnimationObjectFactory::AnimationBuilder::GetKeyframeIndex()
{
	return currentKeyframeIndex;
}

float AnimationObjectFactory::AnimationBuilder::GetKeyframeTime()
{
	return animation.keyframes[currentKeyframeIndex]->timestamp;
}

bool AnimationObjectFactory::AnimationBuilder::CanKeyframeBeRemoved()
{
	return currentKeyframeIndex > 0 && currentKeyframeIndex + 1 < animation.keyframes.size();
}

AnimationObjectFactory::AnimationBuilder& 
AnimationObjectFactory::AnimationBuilder::SetJointTransform(const AnimationTransform& transform)
{
	p_currentJoint->localTransform.position = transform.position;
	p_currentJoint->localTransform.rotation = transform.eulerAngles;
	p_currentJoint->localTransform.scale = transform.scale;
	p_currentJoint->eulerAngles = transform.eulerAngles;
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

AnimationTransform AnimationObjectFactory::AnimationBuilder::GetJointTransform() const
{
	return AnimationTransform(
		p_currentJoint->localTransform.position,
		p_currentJoint->eulerAngles,
		p_currentJoint->localTransform.scale
	);
}

size_t AnimationObjectFactory::AnimationBuilder::GetChildCount() const
{
	return p_currentJoint->childCount;
}

bool AnimationObjectFactory::AnimationBuilder::HasParent() const
{
	return p_currentJoint->p_parent != nullptr;
}

void BuildJointNodes(
	const Engine::Joint& starLeftJoint,
	const Engine::Joint& startRightJoint,
	const glm::mat4& parentWorldTransform,
	const Engine::Joint* p_currentJoint,
	float alpha,
	std::vector<JointNode>& positions
)
{
	glm::mat4 worldTransform = parentWorldTransform *
		Lerp(starLeftJoint.localTransform, startRightJoint.localTransform, alpha).Matrix();
	bool isCurrentJoint = &starLeftJoint == p_currentJoint;

	positions.emplace_back(isCurrentJoint, glm::vec3(worldTransform[3]));

	for (size_t i = 0; i < starLeftJoint.childCount; i++)
	{
		BuildJointNodes(
			starLeftJoint.p_children[i],
			startRightJoint.p_children[i],
			worldTransform,
			p_currentJoint,
			alpha,
			positions
		);
	}
}

void AnimationObjectFactory::AnimationBuilder::GetJointNodes(
	float time,
	std::vector<JointNode>& nodes
) const
{
	nodes.clear();

	if (time == 1.f)
	{
		BuildJointNodes(
			animation.keyframes.back()->skeleton.root,
			animation.keyframes.back()->skeleton.root,
			glm::mat4(1.f),
			p_currentJoint,
			1.f,
			nodes
		);
		return;
	}

	for (size_t i = 1; i < animation.keyframes.size(); i++)
	{
		if (animation.keyframes[i]->timestamp > time)
		{
			float alpha = (time - animation.keyframes[i - 1]->timestamp) /
				(animation.keyframes[i]->timestamp - animation.keyframes[i - 1]->timestamp);

			BuildJointNodes(
				animation.keyframes[i - 1]->skeleton.root,
				animation.keyframes[i]->skeleton.root,
				glm::mat4(1.f),
				p_currentJoint,
				alpha,
				nodes
			);
			break;
		}
	}
}

size_t AnimationObjectFactory::AnimationBuilder::GetJointCount() const
{
	return p_factory->p_state->skeleton.jointCount;
}

const Engine::BindPose& AnimationObjectFactory::AnimationBuilder::GetBindPose() const
{
	return p_factory->p_state->animationObject->bindPose;
}

const Engine::AnimationPose& AnimationObjectFactory::AnimationBuilder::GetAnimationPose(float time) const
{
	animation.GetAnimationPose(
		time, 
		p_factory->p_state->animationObject->bindPose, 
		p_factory->p_state->animationObject->animationPose
	);

	return p_factory->p_state->animationObject->animationPose;
}

AnimationObjectFactory& AnimationObjectFactory::AnimationBuilder::Complete()
{
	animation.BuildAnimation(p_factory->p_state->animationObject->animation);

	p_factory->p_state->stage = AnimationObjectFactory::Stage::AnimationCompleted;

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

AnimationObjectFactory::State::State() :
	stage(Stage::None),
	p_builder(nullptr)
{}

AnimationObjectFactory::Stage AnimationObjectFactory::CurrentStage() const
{
	if (p_state == nullptr)
		return Stage::None;
	
	return p_state->stage;
}

AnimationObjectFactory::SkeletonBuilder& AnimationObjectFactory::StartBuildingSkeleton()
{
	assert(p_state == nullptr);

	p_state = new State();
	p_state->stage = Stage::BuildingSkeleton;
	p_state->animationObject = std::make_shared<AnimationObject>();

	SkeletonBuilder* p_builder = new SkeletonBuilder(this);
	p_state->p_builder = p_builder;

	return *p_builder;
}

AnimationObjectFactory::SkeletonBuilder& AnimationObjectFactory::GetSkeletonBuilder()
{
	assert(p_state != nullptr && p_state->stage == Stage::BuildingSkeleton);
	return *dynamic_cast<SkeletonBuilder*>(p_state->p_builder);
}

AnimationObjectFactory::AnimationBuilder& AnimationObjectFactory::StartAnimating()
{
	assert(p_state != nullptr && p_state->stage == Stage::SkeletonCompleted);
	
	delete p_state->p_builder;
	p_state->stage = Stage::Animating;

	AnimationBuilder* p_builder = new AnimationBuilder(this);
	p_state->p_builder = p_builder;

	return *p_builder;
}

AnimationObjectFactory::AnimationBuilder& AnimationObjectFactory::GetAnimationBuilder()
{
	assert(p_state != nullptr && p_state->stage == Stage::Animating);
	return *dynamic_cast<AnimationBuilder*>(p_state->p_builder);
}

std::shared_ptr<AnimationObject> AnimationObjectFactory::CompleteObject()
{
	assert(p_state != nullptr && p_state->stage == Stage::AnimationCompleted);

	std::shared_ptr<AnimationObject> animationObject = p_state->animationObject;

	delete p_state->p_builder;
	delete p_state;
	p_state = nullptr;

	return animationObject;
}