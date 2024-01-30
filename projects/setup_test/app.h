#include "window.h"
#include "camera.h"
#include "marching_cubes.h"
#include "shader.h"
#include "animation.h"

class App_SetupTest
{
public:
	Engine::Window window;
	Engine::Camera camera;
	Engine::RenderMesh sdfMesh;
	Engine::Shader sdfShader;
	Engine::Bone bindSkeleton;
	Engine::Bone animSkeleton;
	Engine::SkeletonBindPose bindPose;
	Engine::SkeletonAnimationPose animationPose;

	App_SetupTest();

	void Init();
	void UpdateLoop();
	void Deinit();
};