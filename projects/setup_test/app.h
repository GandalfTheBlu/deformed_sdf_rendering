#include "window.h"
#include "camera.h"
#include "marching_cubes.h"
#include "shader.h"

class App_SetupTest
{
public:
	Engine::Window window;
	Engine::Camera camera;
	Engine::RenderMesh mesh;
	Engine::Shader shader;

	App_SetupTest();

	void Init();
	void UpdateLoop();
	void Deinit();
};