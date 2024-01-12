#include "app.h"
#include <glm.hpp>
#include <gtx/quaternion.hpp>
#include "file_watcher.h"
#include "input.h"

float SdBox(const glm::vec3& p, const glm::vec3& b)
{
	glm::vec3 q = glm::abs(p) - b;
	return glm::length(glm::max(q, 0.f)) + glm::min(glm::max(q.x, glm::max(q.y, q.z)), 0.f);
}

float Sdf(const glm::vec3& p)
{
	float box = SdBox(p, glm::vec3(1.f)) - 0.1;
	float sphere = glm::length(p) - 1.4f;
	return glm::min(box, sphere);
}

App_SetupTest::App_SetupTest()
{}

void App_SetupTest::Init()
{
	window.Init(800, 600, "deformed sdf");

	camera.Init(70.f, float(window.Width()) / window.Height(), 0.3f, 500.f);
	
	Engine::TriangulateScalarField(
		mesh, 
		Sdf, 
		glm::vec3(-2.2f), 
		glm::vec3(2.2f), 
		0.4f, 
		glm::length(glm::vec3(0.4f))
	);
	mesh.primitiveGroups[0].mode = GL_PATCHES;
	glPatchParameteri(GL_PATCH_VERTICES, 3);

	shader.Reload(
		"assets/shaders/deform_vert.glsl", 
		"assets/shaders/deform_frag.glsl", 
		{ 
			"assets/shaders/deform_tess_control.glsl", 
			"assets/shaders/deform_tess_eval.glsl"
		}
	);
}

struct Kelvinlet
{
	glm::vec3 center;
	glm::vec3 force;
};

void App_SetupTest::UpdateLoop()
{
	Engine::FileWatcher shaderWatchers[4];
	shaderWatchers[0].Init("assets/shaders/deform_vert.glsl");
	shaderWatchers[1].Init("assets/shaders/deform_frag.glsl");
	shaderWatchers[2].Init("assets/shaders/deform_tess_control.glsl");
	shaderWatchers[3].Init("assets/shaders/deform_tess_eval.glsl");


	glm::mat4 cameraTransform(1.f);
	cameraTransform[3] = glm::vec4(1.f, 0.f, -5.f, 1.f);
	float cameraYaw = 0.f;
	float cameraPitch = 0.f;
	float sensitivity = 0.2f;
	float cameraSpeed = 3.f;

	Kelvinlet kelv{
		glm::vec3(0.f, 0.f, 0.f), 
		glm::vec3(0.f, 0.f, 0.f)
	};

	float pixelRadius = glm::length(glm::vec2(2.f / window.Width(), 2.f / window.Height()));

	bool showDebugMesh = false;

	float totalTime = 0.f;
	float fixedDeltaTime = 1.f / 60.f;

	while (!window.ShouldClose())
	{
		totalTime += fixedDeltaTime;

		window.BeginUpdate();


		for (size_t i = 0; i < 4; i++)
		{
			if (shaderWatchers[i].NewVersionAvailable())
			{
				shader.Reload(
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
				cameraYaw += (float)mouse.movement.dx * sensitivity * fixedDeltaTime;
				cameraPitch += (float)mouse.movement.dy * sensitivity * fixedDeltaTime;
			}

			cameraPitch = glm::clamp(cameraPitch, -1.5f, 1.5f);

			glm::vec3 camPosition = cameraTransform[3];
			glm::quat camRotation = glm::quat(glm::vec3(cameraPitch, cameraYaw, 0.f));

			cameraTransform = glm::mat4(1.f);
			cameraTransform[3] = glm::vec4(camPosition + move * (fixedDeltaTime * cameraSpeed), 1.f);
			cameraTransform = cameraTransform * glm::mat4_cast(camRotation);
		}

		glm::mat4 VP = camera.CalcP() * camera.CalcV(cameraTransform);

		{
			glm::mat4 M(1.f);
			M = glm::rotate(M, totalTime * 0.2f, glm::vec3(0.f, 1.f, 0.f));
			glm::mat4 invM = glm::inverse(M);
			glm::mat3 N = glm::transpose(invM);
			glm::mat4 MVP = VP * M;
			glm::vec3 localCamPos = invM * cameraTransform[3];

			shader.Use();
			shader.SetInt("u_debug", 0);
			shader.SetMat3("u_N", &N[0][0]);
			shader.SetMat4("u_MVP", &MVP[0][0]);
			shader.SetVec3("u_localCameraPos", &localCamPos[0]);
			shader.SetFloat("u_pixelRadius", pixelRadius);
			shader.SetVec3("u_localKelvinletCenter", &kelv.center[0]);
			shader.SetVec3("u_localKelvinletForce", &kelv.force[0]);
			mesh.Bind();
			mesh.Draw(0);

			if (showDebugMesh)
			{
				shader.SetInt("u_debug", 1);
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				mesh.Draw(0);
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			}

			mesh.Unbind();
			shader.StopUsing();
		}

		{
			ImGui::Begin("Deformation");

			if (ImGui::RadioButton("Show mesh", showDebugMesh))
			{
				showDebugMesh = !showDebugMesh;
			}
			ImGui::DragFloat3("Center", &kelv.center.x, 0.1f, -10.f, 10.f, "%.1f", 1.f);
			ImGui::DragFloat3("Force", &kelv.force.x, 0.1f, -10.f, 10.f, "%.1f", 1.f);

			ImGui::End();
		}

		window.EndUpdate();
	}
}

void App_SetupTest::Deinit()
{
	window.Deinit();
}
