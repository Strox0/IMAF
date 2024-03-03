#include "IMAF/Application.h"
#include "IMAF/Panel.h"
#include "IMAF/Scale.h"
#include <windows.h>

#include "imgui.h"
#include "imgui_internal.h"

#include "IMAF/fonts.h"

class ExampleLayer : public IMAF::Panel
{
public:
	void UiRender() override
	{
		ImGui::Begin("Hello");
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::End();

		ImGui::Begin("Something");
		ImGui::Button("Click me", { Pixs(200.f,100.f) });
		ImGui::End();

		ImGui::ShowDemoWindow();
	}
};

void dock_setup(ImGuiID dockspace_id,bool& firstrun)
{
	ImGui::DockBuilderRemoveNode(dockspace_id); // Clear out existing layout
	ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace); // Add empty node
	ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->Size);

	ImGuiID dock_id_prop = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.20f, NULL, &dockspace_id);
	ImGuiID dock_id_bottom = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Down, 0.20f, NULL, &dockspace_id);

	ImGui::DockBuilderDockWindow("Something", dock_id_bottom);
	ImGui::DockBuilderDockWindow("Hello", dock_id_prop);
	ImGui::DockBuilderFinish(dockspace_id);
	firstrun = false;
}

int WINAPI wWinMain(HINSTANCE hInstance,HINSTANCE prevhInstance,PWSTR pCmdLine,int cmdShow)
{
	IMAF::AppProperties props;
	props.resizeable = true;
	props.imgui_docking = true;
	props.gen_ini = false;
	
	IMAF::Application* app = new IMAF::Application(props);

	app->AddScaleCallback(IMAF::Scale::ScaleCallback);

	std::shared_ptr<ExampleLayer> ptr = std::make_shared<ExampleLayer>();
	app->AddPanel(ptr);
	
	app->SetUp([]() 
		{
			ImFontConfig fontConfig;
			fontConfig.FontDataOwnedByAtlas = false;
			ImGui::GetIO().Fonts->AddFontFromMemoryTTF((void*)&g_FontRegular[0], sizeof(g_FontRegular), 27, &fontConfig);
		}
	);

	app->AddDefDockingSetup(dock_setup);

	app->Run();

	delete app;

	return 0;
}