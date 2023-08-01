#include "Application.h"
#include "Panel.h"
#include <windows.h>

class ExampleLayer : public IMAF::Panel
{
public:
	void UiRender() override
	{
		ImGui::Begin("Hello");
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::End();

		ImGui::ShowDemoWindow();
	}
};

int WINAPI wWinMain(HINSTANCE hInstance,HINSTANCE prevhInstance,PWSTR pCmdLine,int cmdShow)
{
	IMAF::AppProperties props;
	props.resizeable = true;
	props.imgui_docking = true;

	IMAF::Application* app = new IMAF::Application(props);
	std::shared_ptr<ExampleLayer> ptr = std::make_shared<ExampleLayer>();
	app->AddPanel(ptr);
	
	app->Run();

	delete app;

	return 0;
}