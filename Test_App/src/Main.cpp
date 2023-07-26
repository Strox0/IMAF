#include "Application.h"
#include <windows.h>

int WINAPI wWinMain(HINSTANCE hInstance,HINSTANCE prevhInstance,PWSTR pCmdLine,int cmdShow)
{
	IMAF::AppProperties props;
	props.costum_titlebar = false;
	props.costum_titlebar_area = 0.07f;

	IMAF::Application* app = new IMAF::Application(props);

	app->Run();

	return 0;
}