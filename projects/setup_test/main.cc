#include "app.h"

int main()
{
	App_SetupTest app;
	
	app.Init();
	app.UpdateLoop();
	app.Deinit();

	return 0;
}