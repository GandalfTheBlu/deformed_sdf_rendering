#include "app.h"
#include <glm.hpp>

float Sdf(const glm::vec3& p)
{
	return glm::length(p) - 1.f;
}

App_SetupTest::App_SetupTest()
{}

void App_SetupTest::Init()
{
	window.Init(800, 600, "deformed sdf");

	camera.Init(70.f, float(window.Width()) / window.Height(), 0.3f, 500.f);
	
	Engine::TriangulateScalarField(mesh, Sdf, glm::vec3(-1.4f), glm::vec3(1.4f), 0.2f, glm::length(glm::vec3(0.2f)));
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
	float radius;
};

void App_SetupTest::UpdateLoop()
{
	glm::mat4 cameraTransform(1.f);
	cameraTransform[3] = glm::vec4(1.f, 0.f, -5.f, 1.f);

	Kelvinlet kelv{
		glm::vec3(0.8f, 0.f, 0.f), 
		glm::vec3(2.f, 0.f, 0.f), 
		1.f
	};

	float totalTime = 0.f;
	float fixedDeltaTime = 1.f / 60.f;

	while (!window.ShouldClose())
	{
		totalTime += fixedDeltaTime;

		window.BeginUpdate();

		glm::mat4 VP = camera.CalcP() * camera.CalcV(cameraTransform);

		{
			kelv.force.x = 1.5f + 0.75f * glm::cos(totalTime);
			glm::mat4 M(1.f);
			glm::mat4 MVP = VP * M;
			glm::vec3 color(1.f, 0.5f, 0.5f);
			glm::vec3 localCamPos = cameraTransform[3] - M[3];

			shader.Use();
			shader.SetMat4("u_MVP", &MVP[0][0]);
			shader.SetVec3("u_color", &color[0]);
			shader.SetVec3("u_localKelvinletCenter", &kelv.center[0]);
			shader.SetVec3("u_localKelvinletForce", &kelv.force[0]);
			shader.SetFloat("u_kelvinletRadius", kelv.radius);
			shader.SetVec3("u_localCameraPos", &localCamPos[0]);
			//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			mesh.Bind();
			mesh.Draw(0);
			mesh.Unbind();
			//glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			shader.StopUsing();
		}

		window.EndUpdate();
	}
}

void App_SetupTest::Deinit()
{
	window.Deinit();
}
