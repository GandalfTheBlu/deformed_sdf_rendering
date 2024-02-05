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

struct AnimationBuildingState
{
	std::vector<BuildingJointNode> buildingJointNodes;
	std::vector<JointNode> jointNodes;
	Engine::AnimationPose buildingAnimationPose;
	float currentKeyframeTime;
	bool keyframeIsSelected;
	float newAnimationDuration;
	bool newAnimationLoop;
	std::shared_ptr<AnimationObject> animationObject;

	AnimationBuildingState();
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

	bool showDebugMesh;
	Engine::RenderMesh jointMesh;
	Engine::RenderMesh weightVolumeMesh;
	Engine::Shader flatShader;

	AnimationObjectFactory animationFactory;
	AnimationBuildingState* p_buildingState;

	App_SetupTest();

	void HandleInput(float deltaTime);
	void DrawSDf();
	void DrawAnimationData();
	void DrawUI(float deltaTime);

	void Init();
	void UpdateLoop();
	void Deinit();
};