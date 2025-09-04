#include "IMAF/Application.h"
#include "IMAF/Panel.h"
#include "IMAF/Scale.h"
#include <windows.h>

#include "imgui.h"
#include "imgui_internal.h"

#include "IMAF/fonts.h"
#include "IMAF/Image.h"

class ExampleLayer : public IMAF::Panel
{
public:
	ExampleLayer() : m_image("test.jpg") 
	{
		m_image.SetImageTint(IM_COL32(255, 0, 0, 255));
	}

	void UiRender() override
	{
		IMAF::Begin("Hello");
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::Text("Icons : " ICON_FA_ANCHOR ICON_FA_BANDAGE ICON_FA_HOTEL ICON_FA_HAMMER ICON_FA_KEY ICON_FA_CHECK ICON_FA_USER);
		ImVec2 image_size = ImGui::GetWindowSize();
		image_size.x -= ImGui::GetStyle().WindowPadding.x*2;
		image_size.y = m_image.GetSize().y;
		IMAF::DisplayImGuiImage(m_image,image_size);
		IMAF::End();

		IMAF::Begin("Something");
		
		ImGui::PushFont(IMAF::GetFont(FONT_EXTRABOLD_NORMAL));
		ImGui::Button("Click me", { IMAF::Val(200.f,100.f) });
		ImGui::PopFont();
		ImGui::PushFont(IMAF::GetFont(FONT_LARGE_ICON));
		ImGui::Button(ICON_FA_THUMBS_UP, { IMAF::Val(200.f,100.f) });
		ImGui::PopFont();

		IMAF::End();

		//ImGui::ShowDemoWindow();
	}

private:
	IMAF::Image m_image;
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
	props.width.relative = 0.55f;
	props.height.relative = 0.5f;
	props.relative_size = true;
	props.custom_titlebar = true;

	props.custom_titlebar_props.top_border = IMAF::TopBorder::Thin;

	IMAF::Application* app = new IMAF::Application(props);

	app->SetUp([&app](const IMAF::AppProperties& props, GLFWwindow* window) 
		{
			IMAF::Titlebar_Properties titlebar_props = props.custom_titlebar_props;
			titlebar_props.height = IMAF::Val(40);
			titlebar_props.AddButton(0, IMAF::ButtonSpec(IMAF::Val(30), IMAF::Val(30), IMAF::Val(5), IMAF::ButtonType::Close));
			titlebar_props.AddButton(0, IMAF::ButtonSpec(IMAF::Val(30), IMAF::Val(30), IMAF::Val(5), IMAF::ButtonType::Maximize));
			titlebar_props.AddButton(0, IMAF::ButtonSpec(IMAF::Val(30), IMAF::Val(30), IMAF::Val(5), IMAF::ButtonType::Minimize), 0.001f, IMAF::GroupAlign::Right, IMAF::Val(5));
			app->SetTitlebarProperties(titlebar_props);
		}
	);

	std::shared_ptr<ExampleLayer> ptr = std::make_shared<ExampleLayer>();
	app->AddPanel(ptr);

	app->AddDefDockingSetup(dock_setup);

	app->Run();

	delete app;

	return 0;
}