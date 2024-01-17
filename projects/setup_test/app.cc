#include "app.h"
#include <glm.hpp>
#include <gtx/quaternion.hpp>
#include "file_watcher.h"
#include "input.h"
#include "default_meshes.h"

//#define KELVINLET_MODE
#define BONE_MODE

glm::vec3 Fold(const glm::vec3& p, const glm::vec3& normal)
{
	return p - 2.f * glm::min(0.f, glm::dot(p, normal)) * normal;
}

glm::vec3 RotX(const glm::vec3& p, float angle)
{
	float startAngle = glm::atan(p.y, p.z);
	float radius = glm::length(glm::vec2(p.y, p.z));
	return glm::vec3(p.x, radius * glm::sin(startAngle + angle), radius * glm::cos(startAngle + angle));
}

float Capsule(const glm::vec3& p, const glm::vec3& a, const glm::vec3& b, float radius)
{
	glm::vec3 pa = p - a;
	glm::vec3 ba = b - a;
	float h = glm::clamp(glm::dot(pa, ba) / glm::dot(ba, ba), 0.f, 1.f);
	return glm::length(pa - ba * h) - radius;
}

float Tree(glm::vec3 p)
{
	glm::vec2 dim = glm::vec2(1.f, 8.f);
	float d = Capsule(p, glm::vec3(0.f, -1.f, 0.f), glm::vec3(0.f, 1.f + dim.y, 0.f), dim.x);
	glm::vec3 scale = glm::vec3(1.f);
	glm::vec3 change = glm::vec3(0.7f, 0.68f, 0.7f);

	const glm::vec3 n1 = normalize(glm::vec3(1.f, 0.f, 1.f));
	const glm::vec3 n2 = glm::vec3(n1.x, 0.f, -n1.z);
	const glm::vec3 n3 = glm::vec3(-n1.x, 0.f, n1.z);

	for (int i = 0; i < 7; i++)
	{
		p = Fold(p, n1);
		p = Fold(p, n2);
		p = Fold(p, n3);

		p.y -= scale.y * dim.y;
		p.z = abs(p.z);
		p = RotX(p, 3.1415f * 0.25f);
		scale *= change;

		d = glm::min(d, Capsule(p, glm::vec3(0.f), glm::vec3(0.f, dim.y * scale.y, 0.), scale.x * dim.x));
	}

	return d;
}

float Sdf(const glm::vec3& p)
{
	return Tree(p * 5.f) / 5.f;
}

App_SetupTest::App_SetupTest()
{}

void App_SetupTest::Init()
{
	window.Init(800, 600, "deformed sdf");
	//window.Init(1600, 1200, "deformed sdf");

	camera.Init(70.f, float(window.Width()) / window.Height(), 0.3f, 500.f);

	Engine::TriangulateScalarField(
		sdfMesh,
		Sdf,
		glm::vec3(-6.f),
		glm::vec3(6.f),
		0.25f,
		glm::length(glm::vec3(0.25f))
	);
	sdfMesh.primitiveGroups[0].mode = GL_PATCHES;
	glPatchParameteri(GL_PATCH_VERTICES, 3);

	sdfShader.Reload(
		"assets/shaders/deform_vert.glsl",
		"assets/shaders/deform_frag.glsl",
		{
			"assets/shaders/deform_tess_control.glsl",
			"assets/shaders/deform_tess_eval.glsl"
		}
	);

	Engine::GenerateUnitSphere(sphereMesh);

	sphereShader.Reload(
		"assets/shaders/flat_vert.glsl",
		"assets/shaders/flat_frag.glsl"
	);
}

struct Kelvinlet
{
	glm::vec3 center;
	glm::vec3 force;
	float sharpness;
};

template<size_t MAX_BONES>
struct SkeletonState
{
	glm::vec3 undeformedBonePoints[MAX_BONES];
	glm::vec2 boneFalloffs[MAX_BONES];
	glm::mat4 boneMatrices[MAX_BONES];
};

template<size_t MAX_BONES>
void InitSkeletonState(SkeletonState<MAX_BONES>& skeletonState)
{
	for (size_t i = 0; i < MAX_BONES; i++)
	{
		skeletonState.undeformedBonePoints[i] = glm::vec3(0.f);
		skeletonState.boneFalloffs[i] = glm::vec2(1.f);
		skeletonState.boneMatrices[i] = glm::mat4(1.f);
	}
}

template<size_t MAX_BONES>
struct SkeletonBuildData
{
	glm::mat4 undeformedBoneTransforms[MAX_BONES];
	glm::mat4 deformedBoneTransforms[MAX_BONES];
};

template<size_t MAX_BONES>
void InitSkeletonBuildData(SkeletonBuildData<MAX_BONES>& skeletonBuildData)
{
	for (size_t i = 0; i < MAX_BONES; i++)
	{
		skeletonBuildData.undeformedBoneTransforms[i] = glm::mat4(1.f);
		skeletonBuildData.deformedBoneTransforms[i] = glm::mat4(1.f);
	}
}

void App_SetupTest::UpdateLoop()
{
	Engine::FileWatcher shaderWatchers[5];
	shaderWatchers[0].Init("assets/shaders/deform_vert.glsl");
	shaderWatchers[1].Init("assets/shaders/deform_frag.glsl");
	shaderWatchers[2].Init("assets/shaders/deform_tess_control.glsl");
	shaderWatchers[3].Init("assets/shaders/deform_tess_eval.glsl");
	shaderWatchers[4].Init("assets/shaders/deformation.glsl");


	glm::mat4 cameraTransform(1.f);
	cameraTransform[3] = glm::vec4(1.f, 0.f, -5.f, 1.f);
	float cameraYaw = 0.f;
	float cameraPitch = 0.f;
	float sensitivity = 0.2f;
	float cameraSpeed = 3.f;

#ifdef KELVINLET_MODE
	Kelvinlet kelv{
		glm::vec3(0.f, 0.f, 0.f),
		glm::vec3(0.f, 0.f, 0.f),
		6.f
	};
#endif

#ifdef BONE_MODE
	constexpr size_t boneCount = 2;

	SkeletonBuildData<boneCount> skeletonBuildData;
	InitSkeletonBuildData(skeletonBuildData);
	skeletonBuildData.undeformedBoneTransforms[1][3] = glm::vec4(0.f, 4.f, 0.f, 1.f);

	SkeletonState<boneCount> skeletonState;
	InitSkeletonState(skeletonState);

	int boneIndex = 0;
	glm::vec3 scales[]{
		glm::vec3(1.f),
		glm::vec3(1.f)
	};
	glm::vec3 eulerAngles[]{
		glm::vec3(0.f),
		glm::vec3(0.f)
	};
	glm::vec3 translations[]{
		glm::vec3(0.f),
		glm::vec3(0.f)
	};
#endif

	//float pixelRadius = glm::length(glm::vec2(2.f / window.Width(), 2.f / window.Height()));

	bool showDebugMesh = true;
	bool showBones = true;

	double time0 = glfwGetTime();

	while (!window.ShouldClose())
	{
		double time1 = glfwGetTime();
		float deltaTime = static_cast<float>(time1 - time0);
		time0 = time1;

		window.BeginUpdate();


		for (size_t i = 0; i < 5; i++)
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
				break;
			}
		}

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

			glm::vec3 move = cameraTransform[0] * axis.x + cameraTransform[2] * axis.z + glm::vec4(0.f, axis.y, 0.f, 0.f);

			if (mouse.rightButton.IsDown())
			{
				cameraYaw += (float)mouse.movement.dx * sensitivity * deltaTime;
				cameraPitch += (float)mouse.movement.dy * sensitivity * deltaTime;
			}

			cameraPitch = glm::clamp(cameraPitch, -1.5f, 1.5f);

			glm::vec3 camPosition = cameraTransform[3];
			glm::quat camRotation = glm::quat(glm::vec3(cameraPitch, cameraYaw, 0.f));

			cameraTransform = glm::mat4(1.f);
			cameraTransform[3] = glm::vec4(camPosition + move * (deltaTime * cameraSpeed), 1.f);
			cameraTransform = cameraTransform * glm::mat4_cast(camRotation);
		}

		glm::mat4 VP = camera.CalcP() * camera.CalcV(cameraTransform);

		{
			glm::mat4 M(1.f);
			glm::mat4 invM = glm::inverse(M);
			glm::mat3 N = glm::transpose(invM);
			glm::mat4 MVP = VP * M;
			glm::vec3 localCamPos = invM * cameraTransform[3];

#ifdef BONE_MODE
			// build skeleton state
			for (size_t i = 0; i < boneCount; i++)
			{
				glm::mat4 scale(1.f);
				scale[0][0] = scales[i][0];
				scale[1][1] = scales[i][1];
				scale[2][2] = scales[i][2];
				glm::mat4 rotate = glm::mat4_cast(glm::quat(eulerAngles[i]));
				glm::mat4 translate(1.f);
				translate[3] = glm::vec4(glm::vec3(skeletonBuildData.undeformedBoneTransforms[i][3]) + translations[i], 1.f);

				skeletonBuildData.deformedBoneTransforms[i] = translate * rotate * scale;

				skeletonState.boneMatrices[i] =
					skeletonBuildData.deformedBoneTransforms[i] *
					glm::inverse(skeletonBuildData.undeformedBoneTransforms[i]);

				skeletonState.undeformedBonePoints[i] = glm::vec3(skeletonBuildData.undeformedBoneTransforms[i][3]);
			}
#endif
			sdfShader.Use();
			sdfShader.SetInt("u_renderMode", 0);
			sdfShader.SetMat3("u_N", &N[0][0]);
			sdfShader.SetMat4("u_MVP", &MVP[0][0]);
			sdfShader.SetVec3("u_localCameraPos", &localCamPos[0]);
			//sdfShader.SetFloat("u_pixelRadius", pixelRadius);

#ifdef KELVINLET_MODE
			sdfShader.SetVec3("u_localKelvinletCenter", &kelv.center[0]);
			sdfShader.SetVec3("u_localKelvinletForce", &kelv.force[0]);
			sdfShader.SetFloat("u_kelvinletSharpness", kelv.sharpness);
#endif
#ifdef BONE_MODE
			sdfShader.SetVec3("u_undeformedBonePoints", &skeletonState.undeformedBonePoints[0][0], boneCount);
			sdfShader.SetVec2("u_boneFalloffs", &skeletonState.boneFalloffs[0][0], boneCount);
			sdfShader.SetMat4("u_boneMatrices", &skeletonState.boneMatrices[0][0][0], boneCount);
#endif
			sdfMesh.Bind();
			sdfMesh.Draw(0);

			if (showDebugMesh)
			{
				sdfShader.SetInt("u_renderMode", 1);
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				sdfMesh.Draw(0);
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			}

			sdfMesh.Unbind();
			sdfShader.StopUsing();
		}

#ifdef BONE_MODE
		if (showBones)
		{
			sphereShader.Use();
			sphereMesh.Bind();
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			glDepthFunc(GL_ALWAYS);

			for (size_t i = 0; i < boneCount; i++)
			{
				glm::mat4 MVP = VP * skeletonBuildData.deformedBoneTransforms[i];
				glm::vec3 color(0.2f);

				if ((int)i == boneIndex)
				{
					color = glm::vec3(1.f, 0.5f, 0.5f);
				}

				sphereShader.SetMat4("u_MVP", &MVP[0][0]);
				sphereShader.SetVec3("u_color", &color[0]);
				sphereMesh.Draw(0);
			}

			glDepthFunc(GL_LESS);
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			sphereMesh.Unbind();
			sphereShader.StopUsing();
		}
#endif

		{
			ImGui::Begin("Menu");

			ImGui::Text("fps: %f", 1.f / deltaTime);

			if (ImGui::RadioButton("Show mesh", showDebugMesh))
				showDebugMesh = !showDebugMesh;

#ifdef BONE_MODE
			if (ImGui::RadioButton("Show bones", showBones))
				showBones = !showBones;

			ImGui::InputInt("Bone index", &boneIndex);
			boneIndex = glm::clamp(boneIndex, 0, (int)boneCount - 1);
			if (ImGui::Button("Reset"))
			{
				scales[boneIndex] = glm::vec3(1.f);
				eulerAngles[boneIndex] = glm::vec3(0.f);
				translations[boneIndex] = glm::vec3(0.f);
				skeletonState.boneFalloffs[boneIndex] = glm::vec2(0.3f, 1.f);
			}
			ImGui::DragFloat3("Scale", &scales[boneIndex][0], 0.03f, 0.1f, 10.f, "%.2f", 1.f);
			ImGui::DragFloat3("Euler angles", &eulerAngles[boneIndex][0], 0.03f, -3.14159f, 3.14159f, "%.2f", 1.f);
			ImGui::DragFloat3("Translation", &translations[boneIndex][0], 0.03f, -10.f, 10.f, "%.2f", 1.f);
			ImGui::DragFloat("Radius", &skeletonState.boneFalloffs[boneIndex][0], 0.03f, 0.1f, 20.f, "%.2f", 1.f);
			ImGui::DragFloat("Falloff rate", &skeletonState.boneFalloffs[boneIndex][1], 0.03f, 0.1f, 20.f, "%.2f", 1.f);
#endif
#ifdef KELVINLET_MODE
			ImGui::DragFloat3("Center", &kelv.center.x, 0.03f, -10.f, 10.f, "%.2f", 1.f);
			ImGui::DragFloat3("Force", &kelv.force.x, 0.03f, -10.f, 10.f, "%.2f", 1.f);
			ImGui::DragFloat("Sharpness", &kelv.sharpness, 0.03f, 1.f, 16.f, "%.2f", 1.f);
#endif
			ImGui::End();
		}

		window.EndUpdate();
	}
}

void App_SetupTest::Deinit()
{
	window.Deinit();
}
