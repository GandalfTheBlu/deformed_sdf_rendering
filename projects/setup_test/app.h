#include "window.h"
#include "camera.h"
#include "marching_cubes.h"
#include "shader.h"

class App_SetupTest
{
public:
	Engine::Window window;
	Engine::Camera camera;
	Engine::RenderMesh sdfMesh;
	Engine::Shader sdfShader;
	Engine::RenderMesh sphereMesh;
	Engine::Shader sphereShader;

	App_SetupTest();

	void Init();
	void UpdateLoop();
	void Deinit();
};