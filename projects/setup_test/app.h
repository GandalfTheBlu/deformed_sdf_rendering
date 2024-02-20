#include "window.h"
#include "camera.h"
#include "shader.h"
#include "render_mesh.h"
#include "voxelizer.h"
#include "animation_factory.h"

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

struct AnimationBuildingState
{
	std::vector<BuildingJointNode> buildingJointNodes;
	std::vector<JointNode> jointNodes;
	std::vector<Engine::JointWeightVolume> buildingWorldWeightVolumes;
	Engine::AnimationPose buildingAnimationPose;
	std::shared_ptr<AnimationObject> animationObject;

	size_t currentJointIndex;
	float currentKeyframeTime;
	bool keyframeIsSelected;
	float newAnimationDuration;
	bool newAnimationLoop;

	AnimationBuildingState();
};

struct PerformanceTestParameters
{
	size_t samplesCount;
	float meshCellSize;
	size_t animationObjectIndex;
	float cameraZPos;
	float maxDistanceFromSurface;
	float maxRadius;

	PerformanceTestParameters();
	PerformanceTestParameters(
		size_t _samplesCount,
		float _meshCellSize,
		size_t _animationObjectIndex,
		float _cameraZPos,
		float _maxDistanceFromSurface,
		float _maxRadius
	);
};

struct PerformanceTest
{
	PerformanceTestParameters parameters;
	enum class State
	{
		Setup,
		SkipFrame,
		Sample
	} state;
	float totalTime;
	float minDeltaTime;
	float maxDeltaTime;
	size_t samplesCollected;

	PerformanceTest();
	PerformanceTest(const PerformanceTestParameters& _parameters);
};

class App_SetupTest
{
public:
	Engine::Window window;

	FlyCam flyCam;

	Engine::RenderMesh sdfMesh;
	Engine::Shader sdfShader;
	float maxDistanceFromSurface;
	float maxRadius;
	glm::vec3 meshBoundingBoxSize;

	Engine::Voxelizer voxelizer;
	glm::vec3 volumeMin;
	glm::vec3 volumeMax;
	glm::ivec3 voxelCount;
	glm::vec3 voxelSize;

	bool showDebugMesh;
	Engine::RenderMesh jointMesh;
	Engine::RenderMesh weightVolumeMesh;
	Engine::Shader flatShader;

	AnimationObjectFactory animationFactory;
	AnimationBuildingState* p_buildingState;
	std::vector<std::shared_ptr<AnimationObject>> createdAnimationObjects;
	size_t animationObjectIndex;
	char filepathBuffer[32];

	std::vector<PerformanceTest*> tests;
	size_t currentTestIndex;
	bool isRunningTests;

	bool showUI;

	App_SetupTest();

	void HandleInput(float deltaTime);
	void ReloadSdf();
	void DrawSDf();
	void DrawAnimationData();
	void DrawUI(float deltaTime);

	void WriteAnimationsToFile(const std::string& filepath);
	void ReadAnimationsFromFile(const std::string& filepath);

	void StartTests();
	void UpdateTests(float deltaTime);
	void SaveTestResults();

	void Init();
	void UpdateLoop();
	void Deinit();
};