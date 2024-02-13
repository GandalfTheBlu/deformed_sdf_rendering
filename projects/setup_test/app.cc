#include "app.h"
#include <glm.hpp>
#include <ctime>
#include <regex>
#include "file_watcher.h"
#include "input.h"
#include "default_meshes.h"
#include "marching_cubes.h"
#include "animation_serializer.h"
#include "file_io.h"


FlyCam::FlyCam() :
	pitch(0.f),
	yaw(0.f),
	speed(1.f),
	sensitivity(1.f),
	transform(1.f)
{}


AnimationBuildingState::AnimationBuildingState() :
	currentJointIndex(0),
	currentKeyframeTime(0.f),
	keyframeIsSelected(true),
	newAnimationDuration(5.f),
	newAnimationLoop(true)
{}


PerformanceTestParameters::PerformanceTestParameters() :
	testDuration(0.f),
	meshCellSize(0.f),
	animationObjectIndex(0),
	cameraPosition(0.f),
	maxDistanceFromSurface(0.f),
	maxRadius(0.f)
{}

PerformanceTestParameters::PerformanceTestParameters(
	float _testDuration,
	float _meshCellSize,
	size_t _animationObjectIndex,
	const glm::vec3& _cameraPosition,
	float _maxDistanceFromSurface,
	float _maxRadius
) :
	testDuration(_testDuration),
	meshCellSize(_meshCellSize),
	animationObjectIndex(_animationObjectIndex),
	cameraPosition(_cameraPosition),
	maxDistanceFromSurface(_maxDistanceFromSurface),
	maxRadius(_maxRadius)
{}


PerformanceTest::PerformanceTest() :
	firstFrame(true),
	totalTime(0.f)
{}

PerformanceTest::PerformanceTest(const PerformanceTestParameters& _parameters, size_t startSamplesCapacity) :
	parameters(_parameters),
	firstFrame(true),
	totalTime(0.f)
{
	deltaTimes.reserve(startSamplesCapacity);
}


App_SetupTest::App_SetupTest() :
	maxDistanceFromSurface(0.f),
	maxRadius(0.f),
	volumeMin(-1.f),
	volumeMax(1.f),
	cellSize(0.1f),
	showDebugMesh(false),
	p_buildingState(nullptr),
	animationObjectIndex(0),
	currentTestIndex(0),
	isRunningTests(false)
{
	for (size_t i = 0; i < 32; i++)
		filepathBuffer[i] = '\0';
}

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
	flyCam.transform = glm::mat4_cast(camRotation);
	flyCam.transform[3] = glm::vec4(camPosition + move * (deltaTime * flyCam.speed), 1.f);
}

void App_SetupTest::ReloadSdf()
{
	Engine::Voxelizer voxelizer;
	if (!voxelizer.Reload("assets/shaders/voxelization_compute.glsl") || 
		!sdfShader.Reload(
			"assets/shaders/deform_vert.glsl",
			"assets/shaders/deform_frag.glsl",
			{
				"assets/shaders/deform_tess_control.glsl",
				"assets/shaders/deform_tess_eval.glsl"
			}
		))
	{
		return;
	}

	std::vector<float> sdf;
	GLuint sizeX = 0;
	GLuint sizeY = 0;
	GLuint sizeZ = 0;

	voxelizer.Voxelize(
		volumeMin,
		volumeMax,
		glm::vec3(cellSize),
		sdf,
		sizeX,
		sizeY,
		sizeZ
	);

	Engine::TriangulateScalarField(
		sdf.data(),
		sizeX,
		sizeY,
		sizeZ,
		volumeMin,
		cellSize,
		glm::length(glm::vec3(cellSize)),
		sdfMesh
	);
}

void SetShaderBuildingWeightVolumes
(
	Engine::Shader& shader,
	const std::vector<Engine::JointWeightVolume>& worldWeightVolumes
)
{
	for (size_t i = 0; i < worldWeightVolumes.size(); i++)
	{
		std::string boneWeightName("u_jointWeightVolumes[" + std::to_string(i) + "]");

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
	shader.SetInt("u_jointCount", (GLint)jointCount);

	for (size_t i = 0; i < jointCount; i++)
	{
		std::string indexStr(std::to_string(i));
		std::string jointWeightName("u_jointWeightVolumes[" + indexStr + "]");
		std::string defMatricesName("u_deformationMatrices[" + indexStr + "]");

		const Engine::JointWeightVolume& weightVolume = p_bindPose->p_worldWeightVolumes[i];

		float length = glm::length(weightVolume.startToEnd);
		glm::vec3 direction = weightVolume.startToEnd / length;

		shader.SetVec3(jointWeightName + ".startPoint", &weightVolume.startPoint[0]);
		shader.SetVec3(jointWeightName + ".direction", &direction[0]);
		shader.SetFloat(jointWeightName + ".length", length);
		shader.SetFloat(jointWeightName + ".falloffRate", weightVolume.falloffRate);
		shader.SetMat4(defMatricesName, &p_animationPose->p_deformationMatrices[i][0][0]);
	}
}

void App_SetupTest::DrawSDf()
{
	glm::mat4 P = flyCam.camera.CalcP();
	glm::mat4 invP = glm::inverse(P);
	glm::mat4 VP = P * flyCam.camera.CalcV(flyCam.transform);
	glm::mat4 invVP = glm::inverse(VP);
	glm::vec3 cameraPos = flyCam.transform[3];
	glm::vec4 nearPlaneBottomLeft = invP * glm::vec4(-1.f, -1.f, -1.f, 1.f);
	glm::vec4 nearPlaneTopRight = invP * glm::vec4(1.f, 1.f, -1.f, 1.f);
	glm::vec2 nearPlaneWorldSize = glm::vec2(
		nearPlaneTopRight.x - nearPlaneBottomLeft.x, 
		nearPlaneTopRight.y - nearPlaneBottomLeft.y
	);
	glm::vec2 screenSize(window.Width(), window.Height());
	glm::vec2 pixelWorldSize = nearPlaneWorldSize / screenSize;
	float pixelRadius = glm::length(pixelWorldSize) * 0.5f;

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

	// draw sdf
	sdfMesh.Bind();
	sdfShader.Use();
	sdfShader.SetInt("u_renderMode", 0);
	sdfShader.SetMat4("u_VP", &VP[0][0]);
	sdfShader.SetMat4("u_invVP", &invVP[0][0]);
	sdfShader.SetVec3("u_cameraPos", &cameraPos[0]);
	sdfShader.SetFloat("u_pixelRadius", pixelRadius);
	sdfShader.SetVec2("u_screenSize", &screenSize[0]);
	sdfShader.SetFloat("u_maxDistanceFromSurface", maxDistanceFromSurface);
	sdfShader.SetFloat("u_maxRadius", maxRadius);

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
	sdfShader.SetInt("u_jointIndex", jointIndex);

	sdfMesh.Draw(0, GL_PATCHES);

	if (showDebugMesh)
	{
		sdfShader.SetInt("u_renderMode", 1);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		sdfMesh.Draw(0, GL_PATCHES);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

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
			glm::mat4 M(0.02f);
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
			glm::mat4 M(0.02f);
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
	ImGui::NewLine();

	if (ImGui::Button("Open"))
		ReadAnimationsFromFile(filepathBuffer);

	ImGui::SameLine();

	if (ImGui::Button("Save"))
		WriteAnimationsToFile(filepathBuffer);

	ImGui::InputText("filepath", filepathBuffer, 32);
	ImGui::NewLine();

	if (createdAnimationObjects.size() > 0 && ImGui::Button("Run tests"))
	{
		StartTests();
		ImGui::End();
		return;
	}

	ImGui::NewLine();

	if (ImGui::Button("Reload SDF"))
		ReloadSdf();

	ImGui::DragFloat3("volume min", &volumeMin[0], 0.05f, -20.f, 20.f, "%.3f", 1.f);
	ImGui::DragFloat3("volume max", &volumeMax[0], 0.05f, -20.f, 20.f, "%.3f", 1.f);
	ImGui::DragFloat("cell size", &cellSize, 0.01f, 0.01f, 1.f, "%.3f", 1.f);
	ImGui::NewLine();

	ImGui::DragFloat("max tracing distance from surface", &maxDistanceFromSurface, 0.05f, 0.1f, 2.f, "%.3f", 1.f);
	ImGui::DragFloat("max tracing radius", &maxRadius, 0.05f, 0.1f, 2.f, "%.3f", 1.f);
	ImGui::NewLine();

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
		{
			builder.AddChild();
			builder.GoToChild(builder.GetChildCount() - 1);
		}
		
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
		bool change = ImGui::DragFloat3("position", &transform.position[0], 0.01f, -10.f, 10.f, "%.3f", 1.f);
		change |= ImGui::DragFloat3("rotation", &transform.eulerAngles[0], 0.01f, -4.f, 4.f, "%.3f", 1.f);
		change |= ImGui::DragFloat("scale", &transform.scale, 0.01f, 0.01f, 20.f, "%.3f", 1.f);
		ImGui::NewLine();

		if (change)
			builder.SetJointTransform(transform);

		// joint weight volume configuration
		Engine::JointWeightVolume weightVolume = builder.GetJointWeightVolume();

		ImGui::Text("Weight volume:");
		change = ImGui::DragFloat3("start", &weightVolume.startPoint[0], 0.01f, -10.f, 10.f, "%.3f", 1.f);
		change |= ImGui::DragFloat3("direction", &weightVolume.startToEnd[0], 0.01f, -10.f, 10.f, "%.3f", 1.f);
		change |= ImGui::DragFloat("falloff rate", &weightVolume.falloffRate, 1.f, 1.f, 500.f, "%.3f", 1.f);
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
			ImGui::Text("Joint:");

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
			bool change = ImGui::DragFloat3("position", &transform.position[0], 0.01f, -10.f, 10.f, "%.3f", 1.f);
			change |= ImGui::DragFloat3("rotation", &transform.eulerAngles[0], 0.01f, -4.f, 4.f, "%.3f", 1.f);
			change |= ImGui::DragFloat("scale", &transform.scale, 0.01f, 0.01f, 20.f, "%.3f", 1.f);
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

		ImGui::DragFloat("Duration", &p_buildingState->newAnimationDuration, 0.01f, 0.1f, 60.f, "%.3f", 1.f);
		if (ImGui::RadioButton("Loop", p_buildingState->newAnimationLoop))
			p_buildingState->newAnimationLoop = !p_buildingState->newAnimationLoop;
	}

	ImGui::End();
}


void App_SetupTest::WriteAnimationsToFile(const std::string& filepath)
{
	std::vector<char> buffer;

	AppendData<size_t>(createdAnimationObjects.size(), buffer);

	for (auto& animationObject : createdAnimationObjects)
		AppendAnimationObjectToBuffer(*animationObject.get(), buffer);

	Engine::WriteBinaryFile(filepath, buffer, false);
}

void App_SetupTest::ReadAnimationsFromFile(const std::string& filepath)
{
	std::vector<char> buffer;
	size_t bufferIndex = 0;
	Engine::ReadBinaryFile(filepath, buffer);

	size_t objectCount = 0;
	ReadData<size_t>(buffer, bufferIndex, objectCount);

	for (size_t i = 0; i < objectCount; i++)
	{
		std::shared_ptr<AnimationObject> animationObject = std::make_shared<AnimationObject>();
		ReadAnimationObjectFromBuffer(buffer, bufferIndex, *animationObject.get());
		createdAnimationObjects.push_back(animationObject);
	}
}

void App_SetupTest::StartTests()
{
	currentTestIndex = 0;
	isRunningTests = true;
	showDebugMesh = false;

	for (PerformanceTest* p_test : tests)
	{
		p_test->firstFrame = true;
		p_test->totalTime = 0.f;
		p_test->deltaTimes.clear();
	}
}

void App_SetupTest::UpdateTests(float deltaTime)
{
	PerformanceTest* p_test = tests[currentTestIndex];

	// setup new test
	if (p_test->firstFrame)
	{
		p_test->firstFrame = false;

		// set camera position
		flyCam.transform = glm::mat4(1.f);
		flyCam.transform[3] = glm::vec4(p_test->parameters.cameraPosition, 1.f);

		// set rendering parameters
		maxDistanceFromSurface = p_test->parameters.maxDistanceFromSurface;
		maxRadius = p_test->parameters.maxRadius;

		// set animation pose at t=0
		animationObjectIndex = p_test->parameters.animationObjectIndex;
		auto& animationObject = createdAnimationObjects[animationObjectIndex];
		animationObject->Start(1.f, false);
		animationObject->Update(0.f);

		// regenerate mesh based on cell size
		cellSize = p_test->parameters.meshCellSize;
		ReloadSdf();

		// don't meassure this frame, since we did a bunch of file io
		return;
	}

	DrawSDf();

	p_test->deltaTimes.push_back(deltaTime);
	p_test->totalTime += deltaTime;

	if (p_test->totalTime >= p_test->parameters.testDuration)
	{
		currentTestIndex++;

		if (currentTestIndex == tests.size())
		{
			isRunningTests = false;
			SaveTestResults();
		}
	}
}

std::string Vec3ToString(const glm::vec3& v)
{
	return "(" + std::to_string(v.x) + ", " + std::to_string(v.y) + ", " + std::to_string(v.z) + ")";
}

void App_SetupTest::SaveTestResults()
{
	std::string resultStr;

	for (PerformanceTest* p_test : tests)
	{
		resultStr += "parameters:\n";
		resultStr += "\tmesh cell size: " + std::to_string(p_test->parameters.meshCellSize) + "\n";
		resultStr += "\tanimation object index: " + std::to_string(p_test->parameters.animationObjectIndex) + "\n";
		resultStr += "\tcamera position: " + Vec3ToString(p_test->parameters.cameraPosition) + "\n";
		resultStr += "\tmax distance from surface: " + std::to_string(p_test->parameters.maxDistanceFromSurface) + "\n";
		resultStr += "\tmax radius: " + std::to_string(p_test->parameters.maxRadius) + "\n";
		resultStr += "results:\n";
		resultStr += "\tnumber of samples: " + std::to_string(p_test->deltaTimes.size()) + "\n";
		resultStr += "\ttotal time: " + std::to_string(p_test->totalTime) + "\n";
		resultStr += "\tseconds per frame data: ";

		for (size_t i=0; i<p_test->deltaTimes.size(); i++)
			resultStr += (i > 0 ? ", " : "") + std::to_string(p_test->deltaTimes[i]);

		resultStr += "\n\n";
	}

	std::time_t time = std::time(0);
	std::string date = std::ctime(&time);

	std::regex dateRgx("[A-Z][a-z]+\\s([A-Z][a-z]+)\\s(\\d+)\\s(\\d+):(\\d+):(\\d+)\\s(\\d+)");
	std::smatch dateMatch;
	std::regex_search(date, dateMatch, dateRgx);
	std::string month = dateMatch[1];
	std::string day = dateMatch[2];
	std::string hour = dateMatch[3];
	std::string minute = dateMatch[4];
	std::string second = dateMatch[5];
	std::string year = dateMatch[6];

	std::map<std::string, std::string> monthNameToNum
	{
		{"Jan", "1"},
		{"Feb", "2"},
		{"Mar", "3"},
		{"Apr", "4"},
		{"May", "5"},
		{"Jun", "6"},
		{"Jul", "7"},
		{"Aug", "8"},
		{"Sep", "9"},
		{"Oct", "10"},
		{"Nov", "11"},
		{"Dec", "12"}
	};

	std::string filename = 
		"test_result_ " + 
		year + "_" +
		monthNameToNum[month] + "_" + 
		day + "_" + 
		hour + "_" +
		minute + "_" + 
		second + ".txt";

	Engine::WriteTextFile("test_results/" + filename, resultStr, false);
}

void App_SetupTest::Init()
{
	//window.Init(1000, 800, "deformed sdf");
	window.Init(1600, 1200, "deformed sdf");
	glfwSwapInterval(0);

	glPatchParameteri(GL_PATCH_VERTICES, 3);

	flyCam.camera.Init(70.f, float(window.Width()) / window.Height(), 0.3f, 500.f);
	flyCam.transform[3] = glm::vec4(0.f, 0.f, -5.f, 1.f);
	flyCam.sensitivity = 0.2f;
	flyCam.speed = 3.f;

	maxDistanceFromSurface = 2.f;
	maxRadius = 0.2f;

	volumeMin = glm::vec3(-1.5f, -1.75f, -1.5f);
	volumeMax = glm::vec3(1.5f, 1.75f, 1.5f);
	cellSize = 0.05f;

	tests.push_back(new PerformanceTest(PerformanceTestParameters(3.f, 0.05f, 0, glm::vec3(0.f, 0.f, -5.f), 2.f, 0.2f), 5 * 60));
	tests.push_back(new PerformanceTest(PerformanceTestParameters(3.f, 0.05f, 0, glm::vec3(0.f, 0.f, -2.5f), 2.f, 0.2f), 5 * 60));
	tests.push_back(new PerformanceTest(PerformanceTestParameters(3.f, 0.05f, 1, glm::vec3(0.f, 0.f, -5.f), 2.f, 0.2f), 5 * 60));
	tests.push_back(new PerformanceTest(PerformanceTestParameters(3.f, 0.05f, 1, glm::vec3(0.f, 0.f, -2.5f), 2.f, 0.2f), 5 * 60));

	std::string defaultFilepath("animations.anim");
	for (size_t i = 0; i < defaultFilepath.size(); i++)
		filepathBuffer[i] = defaultFilepath[i];

	ReloadSdf();

	Engine::GenerateUnitSphere(jointMesh);
	Engine::GenerateUnitLine(weightVolumeMesh);
	flatShader.Reload("assets/shaders/flat_vert.glsl", "assets/shaders/flat_frag.glsl");
}

void App_SetupTest::UpdateLoop()
{
	Engine::FileWatcher sdfWatcher;
	sdfWatcher.Init("assets/shaders/deformation.glsl");

	double time0 = 0.;

	while (!window.ShouldClose())
	{
		double time1 = glfwGetTime();
		float deltaTime = static_cast<float>(time1 - time0);
		time0 = time1;

		window.BeginUpdate();

		if (isRunningTests)
		{
			UpdateTests(deltaTime);
		}
		else
		{
			if (sdfWatcher.NewVersionAvailable())
			{
				sdfShader.Reload(
					"assets/shaders/deform_vert.glsl",
					"assets/shaders/deform_frag.glsl",
					{
						"assets/shaders/deform_tess_control.glsl",
						"assets/shaders/deform_tess_eval.glsl"
					}
				);
			}

			HandleInput(deltaTime);

			if (animationFactory.CurrentStage() == AnimationObjectFactory::Stage::None && createdAnimationObjects.size() > 0)
				createdAnimationObjects[animationObjectIndex]->Update(deltaTime);

			DrawSDf();
			DrawAnimationData();
			DrawUI(deltaTime);
		}

		window.EndUpdate();
	}
}

void App_SetupTest::Deinit()
{
	delete p_buildingState;

	for (PerformanceTest* p_test : tests)
		delete p_test;

	window.Deinit();
}
