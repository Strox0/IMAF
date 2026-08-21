#include "IMAF/Application.h"
#include "IMAF/Panel.h"
#include "IMAF/Scale.h"
#include <windows.h>
#include "imgui.h"
#include "imgui_internal.h"
#include "IMAF/Image.h"
#include "IMAF/Events.h"
#include <IMAF/IconsFontAwesome6.h>
#include <memory>

constexpr IMAF::Events::EventID IMAGE_TINT_CHANGE = 1;

class ExampleLayer : public IMAF::Panel
{
public:
	ExampleLayer() : m_image("test.jpg") 
	{
		m_esub = IMAF::Events::Subscribe(IMAGE_TINT_CHANGE, [this]() -> IMAF::Events::Propagate 
			{
				this->ChangeTint();
				return IMAF::Events::Propagate::Stop;
			}
		);
	}

	~ExampleLayer()
	{
		IMAF::Events::Unsubscribe(m_esub);
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
	}

	void ChangeTint()
	{
		m_red_tint = !m_red_tint;
		if (m_red_tint)
		{
			m_image.SetImageTint(IM_COL32(255, 0, 0, 255));
		}
		else
		{
			m_image.SetImageTint(IM_COL32(255, 255, 255, 255));
		}
	}

private:
	IMAF::Image m_image;
	bool m_red_tint = false;
	IMAF::Events::EventSubID m_esub;
};

class SecondLayer : public IMAF::Panel
{
public:
	void UiRender() override
	{
		IMAF::Begin("Something");

		ImGui::PushFont(IMAF::GetFont(FONT_EXTRABOLD_NORMAL));
		if (ImGui::Button("Change Image Tint", { IMAF::Val(200.f,100.f) }))
		{
			IMAF::Events::Notify(IMAGE_TINT_CHANGE);
		}
		ImGui::PopFont();
		ImGui::PushFont(IMAF::GetFont(FONT_LARGE_ICON));
		ImGui::Button(ICON_FA_THUMBS_UP, { IMAF::Val(200.f,100.f) });
		ImGui::PopFont();

		IMAF::End();
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

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
	IMAF::AppProperties props;
	props.resizeable = true;
	props.imgui_docking = true;
	props.dpi_aware = true;
	props.width.relative = 0.55f;
	props.height.relative = 0.5f;
	props.relative_size = true;
	props.custom_titlebar = true;
	props.transparency = 0.f;
	props.dockspace_color = IMAF::RGBA2_IMVEC4(28, 28, 28, 0.5f);
	
	props.icons.push_back("./account_circle_16dp_E3E3E3.png");
	props.icons.push_back("./account_circle_32dp_E3E3E3.png");
	props.icons.push_back("./account_circle_48dp_E3E3E3.png");

	props.custom_titlebar_props.top_border = IMAF::TopBorder::Thin;
	props.custom_titlebar_props.height = IMAF::Val(40);
	props.custom_titlebar_props.AddButton(0, IMAF::ButtonSpec(IMAF::Val(30), IMAF::Val(30), IMAF::Val(5), IMAF::ButtonType::Close));
	props.custom_titlebar_props.AddButton(0, IMAF::ButtonSpec(IMAF::Val(30), IMAF::Val(30), IMAF::Val(5), IMAF::ButtonType::Maximize));
	props.custom_titlebar_props.AddButton(0, IMAF::ButtonSpec(IMAF::Val(30), IMAF::Val(30), IMAF::Val(5), IMAF::ButtonType::Minimize));
	props.custom_titlebar_props.SetGroupProperties(0, 0.001f, IMAF::GroupAlign::Right, IMAF::Val(5));

	IMAF::Application& app = IMAF::Application::Create(props);

	std::shared_ptr<ExampleLayer> ptr = std::make_shared<ExampleLayer>();
	std::shared_ptr<SecondLayer> ptr2 = std::make_shared<SecondLayer>();
	app.AddPanel(ptr);
	app.AddPanel(ptr2);

	app.AddDefDockingSetup(dock_setup);

	app.Run();

	return 0;
}