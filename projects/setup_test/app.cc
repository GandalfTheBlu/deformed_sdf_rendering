#include "app.h"
#include <glm.hpp>
#include "file_watcher.h"
#include "input.h"
#include "default_meshes.h"

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


App_SetupTest::App_SetupTest()
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

void SetSkeletonShaderData(
	Engine::Shader& shader,
	size_t jointCount,
	const Engine::BindPose& bindPose,
	const Engine::AnimationPose& animationPose,
	bool applyDeformation
)
{
	if (!applyDeformation)
	{
		shader.SetInt("u_bonesCount", 0);
		return;
	}

	for (size_t i = 0; i < jointCount; i++)
	{
		std::string indexStr(std::to_string(i));
		std::string boneWeightName("u_boneWeightVolumes[" + indexStr + "]");
		std::string boneMatrixName("u_boneMatrices[" + indexStr + "]");

		float lengthSquared = glm::dot(
			bindPose.p_worldWeightVolumes[i].startToEnd[0],
			bindPose.p_worldWeightVolumes[i].startToEnd[0]
		);

		shader.SetVec3(boneWeightName + ".startPoint", &bindPose.p_worldWeightVolumes[i].startPoint[0]);
		shader.SetVec3(boneWeightName + ".startToEnd", &bindPose.p_worldWeightVolumes[i].startToEnd[0]);
		shader.SetFloat(boneWeightName + ".lengthSquared", lengthSquared);
		shader.SetFloat(boneWeightName + ".falloffRate", bindPose.p_worldWeightVolumes[i].falloffRate);
		shader.SetMat4(boneMatrixName, &animationPose.p_deformationMatrices[i][0][0]);
	}
	shader.SetInt("u_bonesCount", (GLint)jointCount);
}

void App_SetupTest::DrawSDf(bool applyDeformation, bool showDebugMesh)
{
	glm::mat4 VP = flyCam.camera.CalcP() * flyCam.camera.CalcV(flyCam.transform);
	glm::mat4 M(1.f);
	glm::mat3 N = glm::transpose(glm::inverse(M));
	glm::mat4 invM = glm::inverse(M);
	glm::mat4 MVP = VP * M;
	glm::vec3 localCamPos = invM * flyCam.transform[3];
	float pixelRadius = glm::length(glm::vec2(2.f / window.Width(), 2.f / window.Height()));
	glm::vec2 screenSize(window.Width(), window.Height());

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

	//SetKelvinletShaderData(backfaceShader, kelvinlet, applyDeformation);

	SetSkeletonShaderData(backfaceShader, animationObject->jointCount, animationObject->bindPose, animationObject->animationPose, applyDeformation);

	
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

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, backfaceRenderTarget.texture);

	//SetKelvinletShaderData(sdfShader, kelvinlet, applyDeformation);

	SetSkeletonShaderData(sdfShader, animationObject->jointCount, animationObject->bindPose, animationObject->animationPose, applyDeformation);


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

/*void App_SetupTest::DrawBones(Engine::Bone& startBone, const glm::mat4& parentWorldTransform, const glm::mat4& VP)
{
	glm::mat4 worldTransform = parentWorldTransform * startBone.localTransform.Matrix();
	glm::mat4 M(0.1f);
	M[3] = worldTransform[3];
	glm::mat4 MVP = VP * M;
	glm::vec3 color(1.f);

	boneShader.SetMat4("u_MVP", &MVP[0][0]);
	boneShader.SetVec3("u_color", &color[0]);

	boneMesh.Draw(0);

	for (size_t i = 0; i < startBone.ChildrenCount(); i++)
		DrawBones(startBone.GetChild(i), worldTransform, VP);
}

void App_SetupTest::DrawSkeleton(Engine::Bone& skeleton)
{
	glm::mat4 VP = flyCam.camera.CalcP() * flyCam.camera.CalcV(flyCam.transform);

	glDisable(GL_DEPTH_TEST);
	glDepthMask(0);
	boneShader.Use();
	boneMesh.Bind();

	DrawBones(skeleton, glm::mat4(1.f), VP);

	boneMesh.Unbind();
	boneShader.StopUsing();
	glDepthMask(1);
	glEnable(GL_DEPTH_TEST);
}*/

void App_SetupTest::Init()
{
	window.Init(1000, 800, "deformed sdf");
	//window.Init(1600, 1200, "deformed sdf");

	glPatchParameteri(GL_PATCH_VERTICES, 3);

	flyCam.camera.Init(70.f, float(window.Width()) / window.Height(), 0.3f, 500.f);
	flyCam.transform[3] = glm::vec4(1.f, 0.f, -5.f, 1.f);
	flyCam.sensitivity = 0.2f;
	flyCam.speed = 3.f;

	Engine::TriangulateScalarField(
		sdfMesh,
		Sdf,
		glm::vec3(-1.f, -2.5f, -1.f),
		glm::vec3(1.f, 2.5f, 1.f),
		0.125f,
		glm::length(glm::vec3(0.125f))
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


	AnimationObjectFactory F;
	animationObject = 
	F.StartBuildingSkeleton().
		AddAndGoToChild().SetJointTransform(Engine::Transform(
			glm::vec3(0.f, 1.f, 0.f), 
			glm::vec3(0.f), 
			1.f)
		).Complete().
	StartAnimating().
		AddAndGoToKeyframe(0.2f).GoToChild(0).SetJointTransform(Engine::Transform(
			glm::vec3(0.f, 1.5f, 0.f), 
			glm::vec3(1.f, 0.f, 0.f), 
			3.f
		)).Complete().
	CompleteObject();

	animationObject->Start(10.f, true);
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

	bool showDebugMesh = true;
	bool applyDeformation = true;

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

		animationObject->Update(deltaTime);

		DrawSDf(applyDeformation, showDebugMesh);


		ImGui::Begin("Menu");
		ImGui::Text("fps: %f", 1.f / deltaTime);
		ImGui::NewLine();

		if (ImGui::RadioButton("Show mesh", showDebugMesh))
			showDebugMesh = !showDebugMesh;

		ImGui::NewLine();

		/*ImGui::DragFloat3("Center", &kelvinlet.center.x, 0.03f, -10.f, 10.f, "%.2f", 1.f);
		ImGui::DragFloat3("Force", &kelvinlet.force.x, 0.03f, -10.f, 10.f, "%.2f", 1.f);
		ImGui::DragFloat("Sharpness", &kelvinlet.sharpness, 0.03f, 1.f, 16.f, "%.2f", 1.f);*/

		ImGui::End();
		

		window.EndUpdate();
	}
}

void App_SetupTest::Deinit()
{
	window.Deinit();
}
