#include "window.h"
#include "camera.h"
#include "marching_cubes.h"
#include "shader.h"
#include "animation_factory.h"

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

	std::shared_ptr<AnimationObject> animationObject;

	App_SetupTest();

	void HandleInput(float deltaTime);
	void DrawSDf(bool applyDeformation, bool showDebugMesh);

	void Init();
	void UpdateLoop();
	void Deinit();
};