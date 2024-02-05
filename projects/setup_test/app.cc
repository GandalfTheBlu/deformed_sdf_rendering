#include "app.h"
#include <glm.hpp>
#include "file_watcher.h"
#include "input.h"
#include "default_meshes.h"

float Box(glm::vec3 p, glm::vec3 b)
{
	glm::vec3 q = glm::abs(p) - b;
	return glm::length(glm::max(q, 0.f)) + glm::min(glm::max(q.x, glm::max(q.y, q.z)), 0.f);
}

float Capsule(const glm::vec3& p, const glm::vec3& a, const glm::vec3& b, float radius)
{
	glm::vec3 pa = p - a;
	glm::vec3 ba = b - a;
	float h = glm::clamp(glm::dot(pa, ba) / glm::dot(ba, ba), 0.f, 1.f);
	return glm::length(pa - ba * h) - radius;
}

float Sdf(const glm::vec3& p)
{
	float torso = Capsule(p, glm::vec3(0.f, -0.5f, 0.f), glm::vec3(0.f, 0.5f, 0.f), 0.25f);
	glm::vec3 mirroredP = glm::vec3(glm::abs(p.x), p.y, p.z);
	float arms = Capsule(mirroredP, glm::vec3(0.25f, 0.5f, 0.f), glm::vec3(0.75f, 0.5f, 0.f), 0.25f);
	float legs = Capsule(mirroredP, glm::vec3(0.25f, -0.5f, 0.f), glm::vec3(0.25f, -1.f, 0.f), 0.25f);

	return glm::min(torso, glm::min(arms, legs));
}


RenderTarget::RenderTarget() :
	framebuffer(0),
	texture(0),
	depthBuffer(0)
{}

RenderTarget::~RenderTarget()
{
	glDeleteFramebuffers(1, &framebuffer);
	glDeleteTextures(1, &texture);
	glDeleteRenderbuffers(1, &depthBuffer);
}


FlyCam::FlyCam() :
	pitch(0.f),
	yaw(0.f),
	speed(1.f),
	sensitivity(1.f),
	transform(1.f)
{}


Kelvinlet::Kelvinlet() :
	center(0.f),
	force(0.f),
	sharpness(1.f)
{}


AnimationBuildingState::AnimationBuildingState() :
	currentJointIndex(0),
	currentKeyframeTime(0.f),
	keyframeIsSelected(true),
	newAnimationDuration(5.f),
	newAnimationLoop(true)
{}


App_SetupTest::App_SetupTest() :
	showDebugMesh(false),
	p_buildingState(nullptr),
	animationObjectIndex(0)
{}

void App_SetupTest::HandleInput(float deltaTime)
{
	auto& IP = Engine::Input::Instance();
	const Engine::Mouse& mouse = IP.GetMouse();

	glm::vec3 axis(0.f);

	if (IP.GetKey(GLFW_KEY_W).IsDown())
		axis.z = 1.f;
	else if (IP.GetKey(GLFW_KEY_S).IsDown())
		axis.z = -1.f;

	if (IP.GetKey(GLFW_KEY_D).IsDown())
		axis.x = 1.f;
	else if (IP.GetKey(GLFW_KEY_A).IsDown())
		axis.x = -1.f;

	if (IP.GetKey(GLFW_KEY_Q).IsDown())
		axis.y = 1.f;
	else if (IP.GetKey(GLFW_KEY_E).IsDown())
		axis.y = -1.f;

	glm::vec3 move = flyCam.transform[0] * axis.x + flyCam.transform[2] * axis.z + glm::vec4(0.f, axis.y, 0.f, 0.f);

	if (mouse.rightButton.IsDown())
	{
		flyCam.yaw -= (float)mouse.movement.dx * flyCam.sensitivity * deltaTime;
		flyCam.pitch -= (float)mouse.movement.dy * flyCam.sensitivity * deltaTime;
	}

	flyCam.pitch = glm::clamp(flyCam.pitch, -1.5f, 1.5f);

	glm::vec3 camPosition = flyCam.transform[3];
	glm::quat camRotation = glm::quat(glm::vec3(flyCam.pitch, flyCam.yaw, 0.f));

	flyCam.transform = glm::mat4(1.f);
	flyCam.transform[3] = glm::vec4(camPosition + move * (deltaTime * flyCam.speed), 1.f);
	flyCam.transform = flyCam.transform * glm::mat4_cast(camRotation);
}

void SetKelvinletShaderData(Engine::Shader& shader, const Kelvinlet& kelvinlet, bool applyDeformation)
{
	if (applyDeformation)
	{
		shader.SetVec3("u_localKelvinletCenter", &kelvinlet.center[0]);
		shader.SetVec3("u_localKelvinletForce", &kelvinlet.force[0]);
		shader.SetFloat("u_kelvinletSharpness", kelvinlet.sharpness);
	}
	else
	{
		glm::vec3 zero(0.f);
		shader.SetVec3("u_localKelvinletCenter", &zero[0]);
		shader.SetVec3("u_localKelvinletForce", &zero[0]);
		shader.SetFloat("u_kelvinletSharpness", 1.f);
	}
}

void SetShaderBuildingWeightVolumes
(
	Engine::Shader& shader,
	const std::vector<Engine::JointWeightVolume>& worldWeightVolumes
)
{
	for (size_t i = 0; i < worldWeightVolumes.size(); i++)
	{
		std::string boneWeightName("u_boneWeightVolumes[" + std::to_string(i) + "]");

		const Engine::JointWeightVolume& weightVolume = worldWeightVolumes[i];

		float length = glm::length(weightVolume.startToEnd);
		glm::vec3 direction = weightVolume.startToEnd / length;

		shader.SetVec3(boneWeightName + ".startPoint", &weightVolume.startPoint[0]);
		shader.SetVec3(boneWeightName + ".direction", &direction[0]);
		shader.SetFloat(boneWeightName + ".length", length);
		shader.SetFloat(boneWeightName + ".falloffRate", weightVolume.falloffRate);
	}
}

void SetShaderSkeletonData(
	Engine::Shader& shader,
	size_t jointCount,
	const Engine::BindPose* p_bindPose,
	const Engine::AnimationPose* p_animationPose
)
{
	for (size_t i = 0; i < jointCount; i++)
	{
		std::string indexStr(std::to_string(i));
		std::string boneWeightName("u_boneWeightVolumes[" + indexStr + "]");
		std::string boneMatrixName("u_boneMatrices[" + indexStr + "]");

		const Engine::JointWeightVolume& weightVolume = p_bindPose->p_worldWeightVolumes[i];

		float length = glm::length(weightVolume.startToEnd);
		glm::vec3 direction = weightVolume.startToEnd / length;

		shader.SetVec3(boneWeightName + ".startPoint", &weightVolume.startPoint[0]);
		shader.SetVec3(boneWeightName + ".direction", &direction[0]);
		shader.SetFloat(boneWeightName + ".length", length);
		shader.SetFloat(boneWeightName + ".falloffRate", weightVolume.falloffRate);
		shader.SetMat4(boneMatrixName, &p_animationPose->p_deformationMatrices[i][0][0]);
	}
	shader.SetInt("u_bonesCount", (GLint)jointCount);
}

void App_SetupTest::DrawSDf()
{
	glm::mat4 VP = flyCam.camera.CalcP() * flyCam.camera.CalcV(flyCam.transform);
	glm::mat4 M(1.f);
	glm::mat3 N = glm::transpose(glm::inverse(M));
	glm::mat4 invM = glm::inverse(M);
	glm::mat4 MVP = VP * M;
	glm::vec3 localCamPos = invM * flyCam.transform[3];
	float pixelRadius = glm::length(glm::vec2(2.f / window.Width(), 2.f / window.Height()));
	glm::vec2 screenSize(window.Width(), window.Height());

	size_t jointCount = 0;
	const Engine::BindPose* p_bindPose = nullptr;
	const Engine::AnimationPose* p_animationPose = nullptr;

	if (animationFactory.CurrentStage() == AnimationObjectFactory::Stage::Animating)
	{
		auto& builder = animationFactory.GetAnimationBuilder();
		jointCount = builder.GetJointCount();
		p_bindPose = &builder.GetBindPose();
		p_animationPose = &builder.GetAnimationPose(p_buildingState->currentKeyframeTime);
	}
	else if(animationFactory.CurrentStage() == AnimationObjectFactory::Stage::None && createdAnimationObjects.size() > 0)
	{
		auto& animationObject = createdAnimationObjects[animationObjectIndex];
		jointCount = animationObject->jointCount;
		p_bindPose = &animationObject->bindPose;
		p_animationPose = &animationObject->animationPose;
	}

	// draw backface to get max distance
	glBindFramebuffer(GL_FRAMEBUFFER, backfaceRenderTarget.framebuffer);
	glClearDepth(0.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDepthFunc(GL_GREATER);
	glCullFace(GL_FRONT);

	sdfMesh.Bind();

	backfaceShader.Use();
	backfaceShader.SetMat4("u_MVP", &MVP[0][0]);
	backfaceShader.SetVec3("u_localCameraPos", &localCamPos[0]);

	SetShaderSkeletonData(backfaceShader, jointCount, p_bindPose, p_animationPose);
	
	sdfMesh.Draw(0, GL_PATCHES);

	backfaceShader.StopUsing();
	glClearDepth(1.f);
	glDepthFunc(GL_LESS);
	glCullFace(GL_BACK);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// draw sdf
	sdfShader.Use();
	sdfShader.SetInt("u_renderMode", 0);
	sdfShader.SetMat3("u_N", &N[0][0]);
	sdfShader.SetMat4("u_MVP", &MVP[0][0]);
	sdfShader.SetVec3("u_localCameraPos", &localCamPos[0]);
	sdfShader.SetFloat("u_pixelRadius", pixelRadius);
	sdfShader.SetVec2("u_screenSize", &screenSize[0]);

	int jointIndex = -1;

	if (animationFactory.CurrentStage() == AnimationObjectFactory::Stage::BuildingSkeleton && p_buildingState->buildingJointNodes.size() > 0)
	{
		jointIndex = (int)p_buildingState->currentJointIndex;
		animationFactory.GetSkeletonBuilder().GetWorldJointWeightVolumes(p_buildingState->buildingWorldWeightVolumes);
		SetShaderBuildingWeightVolumes(sdfShader, p_buildingState->buildingWorldWeightVolumes);
	}
	else
	{
		SetShaderSkeletonData(sdfShader, jointCount, p_bindPose, p_animationPose);
	}
	sdfShader.SetInt("u_boneIndex", jointIndex);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, backfaceRenderTarget.texture);

	sdfMesh.Draw(0, GL_PATCHES);

	if (showDebugMesh)
	{
		sdfShader.SetInt("u_renderMode", 1);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		sdfMesh.Draw(0, GL_PATCHES);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);

	sdfShader.StopUsing();
	sdfMesh.Unbind();
	
}

glm::mat4 AlignMatrix(const glm::vec3& up)
{
	glm::vec3 right(1.f, 0.f, 0.f);
	if (glm::abs(up.x) > 0.99f)
		right = glm::vec3(0.f, 1.f, 0.f);

	glm::vec3 forward = glm::normalize(glm::cross(right, up));
	right = glm::cross(up, forward);

	return glm::mat4(
		glm::vec4(right, 0.f),
		glm::vec4(up, 0.f),
		glm::vec4(forward, 0.f),
		glm::vec4(0.f, 0.f, 0.f, 1.f)
	);
}

void App_SetupTest::DrawAnimationData()
{
	if (animationFactory.CurrentStage() == AnimationObjectFactory::Stage::None)
		return;

	glm::mat4 VP = flyCam.camera.CalcP() * flyCam.camera.CalcV(flyCam.transform);

	flatShader.Use();
	glDisable(GL_DEPTH_TEST);
	glDepthMask(0);

	if (animationFactory.CurrentStage() == AnimationObjectFactory::Stage::BuildingSkeleton)
	{
		auto& builder = animationFactory.GetSkeletonBuilder();
		builder.GetBuildingJointNodes(p_buildingState->buildingJointNodes);

		jointMesh.Bind();

		for (size_t i=0; i < p_buildingState->buildingJointNodes.size(); i++)
		{
			glm::mat4 M(0.1f);
			M[3] = glm::vec4(p_buildingState->buildingJointNodes[i].jointWorldPosition, 1.f);
			glm::mat4 MVP = VP * M;
			glm::vec3 color(0.5f);

			if (p_buildingState->buildingJointNodes[i].isCurrentJoint)
			{
				color = glm::vec3(1.f, 0.7f, 0.2f);
				p_buildingState->currentJointIndex = i;
			}

			flatShader.SetMat4("u_MVP", &MVP[0][0]);
			flatShader.SetVec3("u_color", &color[0]);

			jointMesh.Draw(0);
		}

		jointMesh.Unbind();

		weightVolumeMesh.Bind();

		for (auto& node : p_buildingState->buildingJointNodes)
		{
			glm::mat4 M(1.f);
			M[1][1] = glm::length(node.weightWorldStartToEnd);
			M = AlignMatrix(glm::normalize(node.weightWorldStartToEnd)) * M;
			M[3] = glm::vec4(node.weightWorldStartPoint, 1.f);
			glm::mat4 MVP = VP * M;
			glm::vec3 color(0.5f);

			if (node.isCurrentJoint)
			{
				color = glm::vec3(1.f, 0.7f, 0.2f);
				glLineWidth(2.f);
			}

			flatShader.SetMat4("u_MVP", &MVP[0][0]);
			flatShader.SetVec3("u_color", &color[0]);

			weightVolumeMesh.Draw(0, GL_LINES);
			glLineWidth(1.f);
		}

		weightVolumeMesh.Unbind();
	}
	else if (animationFactory.CurrentStage() == AnimationObjectFactory::Stage::Animating)
	{
		auto& builder = animationFactory.GetAnimationBuilder();
		builder.GetJointNodes(p_buildingState->currentKeyframeTime, p_buildingState->jointNodes);

		jointMesh.Bind();

		for (size_t i=0; i < p_buildingState->jointNodes.size(); i++)
		{
			glm::mat4 M(0.1f);
			M[3] = glm::vec4(p_buildingState->jointNodes[i].jointWorldPosition, 1.f);
			glm::mat4 MVP = VP * M;
			glm::vec3 color(0.5f);

			if (p_buildingState->keyframeIsSelected && p_buildingState->jointNodes[i].isCurrentJoint)
			{
				color = glm::vec3(1.f, 0.7f, 0.2f);
				p_buildingState->currentJointIndex = i;
			}

			flatShader.SetMat4("u_MVP", &MVP[0][0]);
			flatShader.SetVec3("u_color", &color[0]);

			jointMesh.Draw(0);
		}

		jointMesh.Unbind();
	}

	glEnable(GL_DEPTH_TEST);
	glDepthMask(1);
	flatShader.StopUsing();
}

void App_SetupTest::DrawUI(float deltaTime)
{
	ImGui::Begin("Menu");
	ImGui::Text("FPS: %.1f", 1.f / deltaTime);
	
	if (ImGui::RadioButton("Show debug mesh", showDebugMesh))
		showDebugMesh = !showDebugMesh;

	ImGui::NewLine();

	auto stage = animationFactory.CurrentStage();
	if (stage == AnimationObjectFactory::Stage::None)
	{
		for (size_t i=0; i<createdAnimationObjects.size(); i++)
		{
			std::string label = "Play animation " + std::to_string(i);

			if (animationObjectIndex == i)
				label += " <";

			if (ImGui::Button(label.c_str()))
			{
				animationObjectIndex = i;
			}
		}

		if (ImGui::Button("New animation"))
		{
			delete p_buildingState;
			p_buildingState = new AnimationBuildingState();
			animationFactory.StartBuildingSkeleton();
		}
		if (createdAnimationObjects.size() > 0 && ImGui::Button("Remove animation"))
		{
			createdAnimationObjects.erase(createdAnimationObjects.begin() + animationObjectIndex);
			animationObjectIndex = 0;
		}
	}
	else if (stage == AnimationObjectFactory::Stage::BuildingSkeleton)
	{
		auto& builder = animationFactory.GetSkeletonBuilder();

		// skeleton navigation and creation
		ImGui::Text("Skeleton:");

		if (ImGui::Button("Add child joint"))
			builder.AddChild();
		
		if (builder.HasParent() && ImGui::Button("Remove joint"))
			builder.RemoveJointAndGoToParent();

		for (size_t i = 0; i < builder.GetChildCount(); i++)
		{
			std::string label = "Go to child joint " + std::to_string(i);
			if (ImGui::Button(label.c_str()))
				builder.GoToChild(i);
		}

		if (builder.HasParent() && ImGui::Button("Go to parent joint"))
			builder.GoToParent();

		ImGui::NewLine();

		// joint transform configuration
		AnimationTransform transform = builder.GetJointTransform();

		ImGui::Text("Transform:");
		bool change = ImGui::DragFloat3("position", &transform.position[0], 0.05f, -10.f, 10.f, "%.3f", 1.f);
		change |= ImGui::DragFloat3("rotation", &transform.eulerAngles[0], 0.05f, -4.f, 4.f, "%.3f", 1.f);
		change |= ImGui::DragFloat("scale", &transform.scale, 0.05f, 0.01f, 20.f, "%.3f", 1.f);
		ImGui::NewLine();

		if (change)
		{
			builder.SetJointTransform(transform);
		}

		// joint weight volume configuration
		Engine::JointWeightVolume weightVolume = builder.GetJointWeightVolume();

		ImGui::Text("Weight volume:");
		change = ImGui::DragFloat3("start", &weightVolume.startPoint[0], 0.05f, -10.f, 10.f, "%.3f", 1.f);
		change |= ImGui::DragFloat3("direction", &weightVolume.startToEnd[0], 0.05f, -10.f, 10.f, "%.3f", 1.f);
		change |= ImGui::DragFloat("falloff rate", &weightVolume.falloffRate, 0.5f, 1.f, 100.f, "%.3f", 1.f);
		ImGui::NewLine();

		if (change)
			builder.SetJointWeightVolume(weightVolume);

		// skeleton creation
		if (ImGui::Button("Complete skeleton"))
			builder.Complete().StartAnimating();
	}
	else if (stage == AnimationObjectFactory::Stage::Animating)
	{
		auto& builder = animationFactory.GetAnimationBuilder();

		// building and navigating keyframes
		ImGui::Text("Keyframe:");

		if (ImGui::Button("Add new keyframe"))
		{
			builder.AddAndGoToKeyframe(p_buildingState->currentKeyframeTime);
			p_buildingState->keyframeIsSelected = true;
		}

		if (ImGui::SliderFloat("New keyframe timestamp", &p_buildingState->currentKeyframeTime, 0.f, 1.f, "%.3f", 1.f))
			p_buildingState->keyframeIsSelected = false;

		ImGui::NewLine();

		if (builder.CanKeyframeBeRemoved() && ImGui::Button("Remove keyframe"))
			builder.RemoveKeyframeAndGoLeft();

		for (size_t i = 0; i < builder.GetKeyframeCount(); i++)
		{
			std::string label = "Go to keyframe " + std::to_string(i);
			
			if (p_buildingState->keyframeIsSelected && builder.GetKeyframeIndex() == i)
				label += " <";

			if (ImGui::Button(label.c_str()))
			{
				builder.GoToKeyframe(i);
				p_buildingState->currentKeyframeTime = builder.GetKeyframeTime();
				p_buildingState->keyframeIsSelected = true;
			}
		}

		ImGui::NewLine();

		// posing and navigating skeleton
		if (p_buildingState->keyframeIsSelected)
		{
			ImGui::Text("Pose:");

			for (size_t i = 0; i < builder.GetChildCount(); i++)
			{
				std::string label = "Go to child joint " + std::to_string(i);
				if (ImGui::Button(label.c_str()))
					builder.GoToChild(i);
			}

			if (builder.HasParent() && ImGui::Button("Go to parent joint"))
				builder.GoToParent();

			ImGui::NewLine();

			// joint transform configuration
			AnimationTransform transform = builder.GetJointTransform();

			ImGui::Text("Transform:");
			bool change = ImGui::DragFloat3("position", &transform.position[0], 0.05f, -10.f, 10.f, "%.3f", 1.f);
			change |= ImGui::DragFloat3("rotation", &transform.eulerAngles[0], 0.05f, -4.f, 4.f, "%.3f", 1.f);
			change |= ImGui::DragFloat("scale", &transform.scale, 0.05f, 0.01f, 20.f, "%.3f", 1.f);
			ImGui::NewLine();

			if (change)
			{
				builder.SetJointTransform(transform);
			}
		}

		// animation creation
		if (ImGui::Button("Complete animation"))
		{
			p_buildingState->animationObject = builder.Complete().CompleteObject();
			p_buildingState->animationObject->animationPlayer.duration = p_buildingState->newAnimationDuration;
			p_buildingState->animationObject->animationPlayer.loop = p_buildingState->newAnimationLoop;

			createdAnimationObjects.push_back(p_buildingState->animationObject);
			animationObjectIndex = createdAnimationObjects.size() - 1;
		}

		ImGui::DragFloat("Duration", &p_buildingState->newAnimationDuration, 0.02f, 0.1f, 60.f, "%.3f", 1.f);
		if (ImGui::RadioButton("Loop", p_buildingState->newAnimationLoop))
			p_buildingState->newAnimationLoop = !p_buildingState->newAnimationLoop;
	}

	ImGui::End();
}

void App_SetupTest::Init()
{
	//window.Init(1000, 800, "deformed sdf");
	window.Init(1600, 1200, "deformed sdf");

	glPatchParameteri(GL_PATCH_VERTICES, 3);

	flyCam.camera.Init(70.f, float(window.Width()) / window.Height(), 0.3f, 500.f);
	flyCam.transform[3] = glm::vec4(0.f, 0.f, -5.f, 1.f);
	flyCam.sensitivity = 0.2f;
	flyCam.speed = 3.f;

	Engine::TriangulateScalarField(
		sdfMesh,
		Sdf,
		glm::vec3(-1.5f, -1.75f, -1.5f),
		glm::vec3(1.5f, 1.75f, 1.5f),
		0.05f,
		glm::length(glm::vec3(0.05f))
	);
	sdfShader.Reload(
		"assets/shaders/deform_vert.glsl",
		"assets/shaders/deform_frag.glsl",
		{
			"assets/shaders/deform_tess_control.glsl",
			"assets/shaders/deform_tess_eval.glsl"
		}
	);
	backfaceShader.Reload(
		"assets/shaders/deform_vert.glsl",
		"assets/shaders/backface_frag.glsl",
		{
			"assets/shaders/deform_tess_control.glsl",
			"assets/shaders/backface_tess_eval.glsl"
		}
	);

	Engine::GenerateUnitSphere(jointMesh);
	Engine::GenerateUnitLine(weightVolumeMesh);
	flatShader.Reload("assets/shaders/flat_vert.glsl", "assets/shaders/flat_frag.glsl");

	glGenFramebuffers(1, &backfaceRenderTarget.framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, backfaceRenderTarget.framebuffer);

	// add target texture
	glGenTextures(1, &backfaceRenderTarget.texture);
	glBindTexture(GL_TEXTURE_2D, backfaceRenderTarget.texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, window.Width(), window.Height(), 0, GL_RED, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, backfaceRenderTarget.texture, 0);

	GLuint attachments[]{ GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, attachments);

	// add depth buffer
	glGenRenderbuffers(1, &backfaceRenderTarget.depthBuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, backfaceRenderTarget.depthBuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, window.Width(), window.Height());
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, backfaceRenderTarget.depthBuffer);

	assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void App_SetupTest::UpdateLoop()
{
	Engine::FileWatcher shaderWatchers[7];
	shaderWatchers[0].Init("assets/shaders/deform_vert.glsl");
	shaderWatchers[1].Init("assets/shaders/deform_frag.glsl");
	shaderWatchers[2].Init("assets/shaders/deform_tess_control.glsl");
	shaderWatchers[3].Init("assets/shaders/deform_tess_eval.glsl");
	shaderWatchers[4].Init("assets/shaders/deformation.glsl");
	shaderWatchers[5].Init("assets/shaders/backface_frag.glsl");
	shaderWatchers[6].Init("assets/shaders/backface_tess_eval.glsl");

	double time0 = glfwGetTime();

	while (!window.ShouldClose())
	{
		double time1 = glfwGetTime();
		float deltaTime = static_cast<float>(time1 - time0);
		time0 = time1;

		window.BeginUpdate();

		HandleInput(deltaTime);

		for (size_t i = 0; i < 7; i++)
		{
			if (shaderWatchers[i].NewVersionAvailable())
			{
				sdfShader.Reload(
					"assets/shaders/deform_vert.glsl",
					"assets/shaders/deform_frag.glsl",
					{
						"assets/shaders/deform_tess_control.glsl",
						"assets/shaders/deform_tess_eval.glsl"
					}
				);

				backfaceShader.Reload(
					"assets/shaders/deform_vert.glsl",
					"assets/shaders/backface_frag.glsl",
					{
						"assets/shaders/deform_tess_control.glsl",
						"assets/shaders/backface_tess_eval.glsl"
					}
				);

				break;
			}
		}

		if (animationFactory.CurrentStage() == AnimationObjectFactory::Stage::None && createdAnimationObjects.size() > 0)
			createdAnimationObjects[animationObjectIndex]->Update(deltaTime);
		
		DrawSDf();
		DrawAnimationData();
		DrawUI(deltaTime);
		

		window.EndUpdate();
	}
}

void App_SetupTest::Deinit()
{
	delete p_buildingState;
	window.Deinit();
}
