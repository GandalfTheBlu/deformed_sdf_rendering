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
	float testDuration;
	float meshCellSize;
	size_t animationObjectIndex;
	glm::vec3 cameraPosition;
	float maxDistanceFromSurface;
	float maxRadius;

	PerformanceTestParameters();
	PerformanceTestParameters(
		float _testDuration,
		float _meshCellSize,
		size_t _animationObjectIndex,
		const glm::vec3& _cameraPosition,
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
	size_t samplesCount;

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

	Engine::Voxelizer voxelizer;
	glm::vec3 volumeMin;
	glm::vec3 volumeMax;
	float cellSize;

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