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
		IMAF::Begin("Hello");
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		IMAF::End();

		IMAF::Begin("Something");
		
		ImGui::PushFont(IMAF::GetFont(FONT_BOLD_NORMAL));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,IMAF::Val(20,20));
		ImGui::Button("Click me", { IMAF::Val(200.f,100.f) });
		ImGui::Button("Click me", { IMAF::Val(200.f,100.f) });
		ImGui::PopFont();
		ImGui::PopStyleVar();
		IMAF::End();

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
	props.dpi_aware = true;
	props.width.relative = 0.75f;
	props.height.relative = 0.6f;
	props.relative_size = true;
	props.custom_titlebar = true;

	props.custom_titlebar_props.height = 50;
	props.custom_titlebar_props.AddButton(0, IMAF::ButtonSpec(40, 40, 5, IMAF::ButtonType::Close));
	props.custom_titlebar_props.AddButton(0, IMAF::ButtonSpec(40, 40, 5, IMAF::ButtonType::Maximize));
	props.custom_titlebar_props.AddButton(0, IMAF::ButtonSpec(40, 40, 5, IMAF::ButtonType::Minimize), 0.001f, IMAF::GroupAlign::Right, 5);
	props.custom_titlebar_props.top_border = IMAF::TopBorder::Thin;

	IMAF::Application* app = new IMAF::Application(props);

	std::shared_ptr<ExampleLayer> ptr = std::make_shared<ExampleLayer>();
	app->AddPanel(ptr);

	app->AddDefDockingSetup(dock_setup);

	app->Run();

	delete app;

	return 0;
}