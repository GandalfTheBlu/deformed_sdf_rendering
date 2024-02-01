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
	return Box(p, glm::vec3(0.5f, 2.f, 0.5f));
	//return Tree(p * 5.f) / 5.f;
}

App_SetupTest::App_SetupTest()
{
	backfaceRenderTarget = { 0, 0, 0 };
}

void App_SetupTest::Init()
{
	window.Init(1000, 800, "deformed sdf");
	//window.Init(1600, 1200, "deformed sdf");

	camera.Init(70.f, float(window.Width()) / window.Height(), 0.3f, 500.f);

	Engine::TriangulateScalarField(
		sdfMesh,
		Sdf,
		glm::vec3(-1.f, -2.5f, -1.f),
		glm::vec3(1.f, 2.5f, 1.f),
		0.25f,
		glm::length(glm::vec3(0.25f))
	);
	glPatchParameteri(GL_PATCH_VERTICES, 3);

	sdfShader.Reload(
		"assets/shaders/deform_vert.glsl",
		"assets/shaders/deform_frag.glsl",
		{
			"assets/shaders/deform_tess_control.glsl",
			"assets/shaders/deform_tess_eval.glsl"
		}
	);

	// configure bind pose
	bindSkeleton.localTransform = Engine::Transform(glm::vec3(0.f, -1.f, 0.f), glm::vec3(0.f), glm::vec3(1.f));
	bindSkeleton.localWeightVolume.startPoint = glm::vec3(0.f, -1.f, 0.f);
	bindSkeleton.localWeightVolume.startToEnd = glm::vec3(0.f, 2.f, 0.f);
	bindSkeleton.localWeightVolume.falloffRate = 10.f;

	bindSkeleton.AddChild();
	bindSkeleton.GetChild(0).localTransform = Engine::Transform(glm::vec3(0.f, 1.f, 0.f), glm::vec3(0.f), glm::vec3(1.f));
	bindSkeleton.GetChild(0).localWeightVolume.startPoint = glm::vec3(0.f, 1.f, 0.f);
	bindSkeleton.GetChild(0).localWeightVolume.startToEnd = glm::vec3(0.f, 2.f, 0.f);
	bindSkeleton.GetChild(0).localWeightVolume.falloffRate = 10.f;

	bindSkeleton.GenerateBindPose(bindPose, glm::mat4(1.f));

	bindSkeleton.MakeCopy(animSkeleton);


	// setup backface rendering shader and framebuffer
	backfaceShader.Reload(
		"assets/shaders/deform_vert.glsl", 
		"assets/shaders/backface_frag.glsl", 
		{
			"assets/shaders/deform_tess_control.glsl",
			"assets/shaders/backface_tess_eval.glsl"
		}
	);

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

struct Kelvinlet
{
	glm::vec3 center;
	glm::vec3 force;
	float sharpness;
};

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

	float pixelRadius = glm::length(glm::vec2(2.f / window.Width(), 2.f / window.Height()));

	bool showDebugMesh = true;
	
	Engine::Bone* p_focusedBindBone = &bindSkeleton;
	Engine::Bone* p_focusedAnimBone = &animSkeleton;

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
				cameraYaw -= (float)mouse.movement.dx * sensitivity * deltaTime;
				cameraPitch -= (float)mouse.movement.dy * sensitivity * deltaTime;
			}

			cameraPitch = glm::clamp(cameraPitch, -1.5f, 1.5f);

			glm::vec3 camPosition = cameraTransform[3];
			glm::quat camRotation = glm::quat(glm::vec3(cameraPitch, cameraYaw, 0.f));

			cameraTransform = glm::mat4(1.f);
			cameraTransform[3] = glm::vec4(camPosition + move * (deltaTime * cameraSpeed), 1.f);
			cameraTransform = cameraTransform * glm::mat4_cast(camRotation);
		}

		animationPose.worldTransforms.clear();
		animSkeleton.GenerateAnimationPose(animationPose, glm::mat4(1.f));

		glm::mat4 VP = camera.CalcP() * camera.CalcV(cameraTransform);

		// draw backface to get max distance
		{
			glBindFramebuffer(GL_FRAMEBUFFER, backfaceRenderTarget.framebuffer);
			glClearDepth(0.f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glDepthFunc(GL_GREATER);
			glCullFace(GL_FRONT);

			glm::mat4 M(1.f);
			glm::mat4 invM = glm::inverse(M);
			glm::mat4 MVP = VP * M;
			glm::vec3 localCamPos = invM * cameraTransform[3];

			backfaceShader.Use();
			backfaceShader.SetMat4("u_MVP", &MVP[0][0]);
			backfaceShader.SetVec3("u_localCameraPos", &localCamPos[0]);

#ifdef BONE_MODE
			for (size_t i = 0; i < bindPose.weightVolumes.size(); i++)
			{
				std::string indexStr(std::to_string(i));
				std::string boneWeightName("u_boneWeightVolumes[" + indexStr + "]");
				std::string boneMatrixName("u_boneMatrices[" + indexStr + "]");

				glm::mat4 boneMatrix = animationPose.worldTransforms[i] * bindPose.inverseWorldTransforms[i];

				backfaceShader.SetVec3(boneWeightName + ".startPoint", &bindPose.weightVolumes[i].startPoint[0]);
				backfaceShader.SetVec3(boneWeightName + ".startToEnd", &bindPose.weightVolumes[i].startToEnd[0]);
				backfaceShader.SetFloat(boneWeightName + ".lengthSquared", bindPose.weightVolumes[i].lengthSquared);
				backfaceShader.SetFloat(boneWeightName + ".falloffRate", bindPose.weightVolumes[i].falloffRate);
				backfaceShader.SetMat4(boneMatrixName, &boneMatrix[0][0]);
			}
			backfaceShader.SetInt("u_bonesCount", (GLint)bindPose.inverseWorldTransforms.size());
#endif

			sdfMesh.Bind();
			sdfMesh.Draw(0, GL_PATCHES);
			sdfMesh.Unbind();

			backfaceShader.StopUsing();

			glClearDepth(1.f);
			glDepthFunc(GL_LESS);
			glCullFace(GL_BACK);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

		{
			glm::mat4 M(1.f);
			glm::mat4 invM = glm::inverse(M);
			glm::mat3 N = glm::transpose(invM);
			glm::mat4 MVP = VP * M;
			glm::vec3 localCamPos = invM * cameraTransform[3];
			glm::vec2 screenSize(window.Width(), window.Height());

			sdfShader.Use();
			sdfShader.SetInt("u_renderMode", 0);
			sdfShader.SetMat3("u_N", &N[0][0]);
			sdfShader.SetMat4("u_MVP", &MVP[0][0]);
			sdfShader.SetVec3("u_localCameraPos", &localCamPos[0]);
			sdfShader.SetFloat("u_pixelRadius", pixelRadius);
			sdfShader.SetVec2("u_screenSize", &screenSize[0]);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, backfaceRenderTarget.texture);

#ifdef KELVINLET_MODE
			sdfShader.SetVec3("u_localKelvinletCenter", &kelv.center[0]);
			sdfShader.SetVec3("u_localKelvinletForce", &kelv.force[0]);
			sdfShader.SetFloat("u_kelvinletSharpness", kelv.sharpness);
#endif
#ifdef BONE_MODE
			for (size_t i = 0; i < bindPose.weightVolumes.size(); i++)
			{
				std::string indexStr(std::to_string(i));
				std::string boneWeightName("u_boneWeightVolumes[" + indexStr + "]");
				std::string boneMatrixName("u_boneMatrices[" + indexStr + "]");

				glm::mat4 boneMatrix = animationPose.worldTransforms[i] * bindPose.inverseWorldTransforms[i];

				sdfShader.SetVec3(boneWeightName + ".startPoint", &bindPose.weightVolumes[i].startPoint[0]);
				sdfShader.SetVec3(boneWeightName + ".startToEnd", &bindPose.weightVolumes[i].startToEnd[0]);
				sdfShader.SetFloat(boneWeightName + ".lengthSquared", bindPose.weightVolumes[i].lengthSquared);
				sdfShader.SetFloat(boneWeightName + ".falloffRate", bindPose.weightVolumes[i].falloffRate);
				sdfShader.SetMat4(boneMatrixName, &boneMatrix[0][0]);
			}
			sdfShader.SetInt("u_bonesCount", (GLint)bindPose.inverseWorldTransforms.size());
#endif
			sdfMesh.Bind();
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

			sdfMesh.Unbind();
			sdfShader.StopUsing();
		}

		{
			ImGui::Begin("Menu");

			ImGui::Text("fps: %f", 1.f / deltaTime);

			if (ImGui::RadioButton("Show mesh", showDebugMesh))
				showDebugMesh = !showDebugMesh;

#ifdef KELVINLET_MODE
			ImGui::DragFloat3("Center", &kelv.center.x, 0.03f, -10.f, 10.f, "%.2f", 1.f);
			ImGui::DragFloat3("Force", &kelv.force.x, 0.03f, -10.f, 10.f, "%.2f", 1.f);
			ImGui::DragFloat("Sharpness", &kelv.sharpness, 0.03f, 1.f, 16.f, "%.2f", 1.f);
#endif
#ifdef BONE_MODE
			ImGui::NewLine();
			ImGui::Text("Bone:");
			ImGui::DragFloat3("Position", &p_focusedAnimBone->localTransform.position[0], 0.03f, -10.f, 10.f, "%.2f", 1.f);
			ImGui::DragFloat3("Euler angles", &p_focusedAnimBone->localTransform.eulerAngles[0], 0.03f, -3.f, 3.f, "%.2f", 1.f);
			ImGui::DragFloat3("Scale", &p_focusedAnimBone->localTransform.scale[0], 0.03f, 0.1f, 10.f, "%.2f", 1.f);
			if (ImGui::Button("Reset"))
			{
				p_focusedAnimBone->localTransform = p_focusedBindBone->localTransform;
			}

			ImGui::NewLine();
			for (size_t i = 0; i < p_focusedBindBone->ChildrenCount(); i++)
			{
				std::string btnName("Select child " + std::to_string(i));
				if (ImGui::Button(btnName.c_str()))
				{
					p_focusedBindBone = &p_focusedBindBone->GetChild(i);
					p_focusedAnimBone = &p_focusedAnimBone->GetChild(i);
					break;
				}
			}

			if (p_focusedBindBone->p_parent != nullptr && ImGui::Button("Select parent"))
			{
				p_focusedBindBone = p_focusedBindBone->p_parent;
				p_focusedAnimBone = p_focusedAnimBone->p_parent;
			}
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
