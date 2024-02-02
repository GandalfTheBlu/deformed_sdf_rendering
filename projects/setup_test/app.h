#include "window.h"
#include "camera.h"
#include "marching_cubes.h"
#include "shader.h"
#include "animation.h"

#define KELVINLET_MODE
//#define SKELETON_MODE

struct RenderTarget
{
	GLuint framebuffer;
	GLuint texture;
	GLuint depthBuffer;

	RenderTarget();
	~RenderTarget();
};

struct FlyCam
{
	float pitch;
	float yaw;
	float speed;
	float sensitivity;
	glm::mat4 transform;
	Engine::Camera camera;

	FlyCam();
};

struct Kelvinlet
{
	glm::vec3 center;
	glm::vec3 force;
	float sharpness;

	Kelvinlet();
};

class App_SetupTest
{
public:
	Engine::Window window;

	FlyCam flyCam;

	Engine::RenderMesh sdfMesh;
	Engine::Shader sdfShader;
	Engine::Shader backfaceShader;
	RenderTarget backfaceRenderTarget;

#ifdef KELVINLET_MODE
	Kelvinlet kelvinlet;
#endif

#ifdef SKELETON_MODE
	Engine::Bone bindSkeleton;
	Engine::Bone animSkeleton;
	Engine::SkeletonBindPose bindPose;
	Engine::SkeletonAnimationPose animationPose;
	Engine::RenderMesh boneMesh;
	Engine::Shader boneShader;
#endif
	App_SetupTest();

	void HandleInput(float deltaTime);
	void DrawSDf(bool applyDeformation, bool showDebugMesh);

#ifdef SKELETON_MODE
	void DrawBones(Engine::Bone& startBone, const glm::mat4& parentWorldTransform, const glm::mat4& VP);
	void DrawSkeleton(Engine::Bone& skeleton);
#endif

	void Init();
	void UpdateLoop();
	void Deinit();
};