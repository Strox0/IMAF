#pragma once

#include "imgui.h"
#include "GLFW/glfw3.h"

#include <memory>
#include <functional>
#include <mutex>
#include <Windows.h>
#include <array>

#include "Panel.h"
#include "Scale.h"

#define RGB2_IMVEC4(r,g,b) ImVec4( r / 255.0f , g / 255.0f ,b / 255.0f, 1.0f )
#define RGBA2_IMVEC4(r,g,b,a) ImVec4( r / 255.0f , g / 255.0f ,b / 255.0f, a )

#define FONT_DEFAULT				0
#define FONT_NORMAL					0
#define FONT_ICON					1
#define FONT_LARGE_ICON				2
#define FONT_LIGHT_NORMAL			3
#define FONT_MEDIUM_NORMAL			4
#define FONT_SEMIBOLD_NORMAL		5
#define FONT_BOLD_NORMAL			6
#define FONT_EXTRABOLD_NORMAL		7
#define FONT_LAST FONT_EXTRABOLD_NORMAL

struct GLFWwindow;

namespace IMAF 
{
	enum IconFont : unsigned short
	{
		FontAwesome6_Regluar,
		FontAwesome6_Solid
	};

	enum ButtonType : unsigned short
	{
		Close = GLFW_CT_CLOSE_BUTTON,
		Minimize = GLFW_CT_MINIMIZE_BUTTON,
		Maximize = GLFW_CT_MAXIMIZE_BUTTON
	};

	struct ButtonSpec
	{
		ButtonSpec(int width, ButtonType type) : width(width), type(type), height(-1), top_offset(0) {};
		ButtonSpec(int width, int height, int top_offset, ButtonType type) : width(width), height(height), type(type), top_offset(top_offset) {};

		int top_offset;
		int width;
		int height;
		ButtonType type;
	};

	enum GroupAlign : unsigned short
	{
		Null,
		Left = GLFW_CT_ALIGN_LEFT,
		Center = GLFW_CT_ALIGN_CENTER,
		Right = GLFW_CT_ALIGN_RIGHT
	};

	struct ButtonGroup 
	{
		std::vector<ButtonSpec> buttons;
		GroupAlign align = GroupAlign::Right;
		float edge_offset = 0.f;
		int inner_spacing = 0;
	};

	struct ExclusionSpec
	{
		float start_offset;
		int width;
		int height;
		int top_offset;

		ExclusionSpec(float start_offset, int top_offset, int width, int height) : start_offset(start_offset), top_offset(top_offset), width(width), height(height) {};
	};

	enum TopBorder
	{
		None,
		Thin,
		Color,
		Default = None
	};

	struct AppProperties;

	struct Titlebar_Properties
	{
		//edge offset is the distance from the edge of the window that is specified in align to the first button
		//0.f with right align will place the first button at the right edge of the window
		//center align will ignore the edge offset
		void AddButton(unsigned short group_index, const ButtonSpec& button_spec, float edge_offset = 0.f, GroupAlign align = GroupAlign::Null, int spacing = 0)
		{
			if (button_groups.size() > group_index)
			{
				button_groups[group_index].buttons.push_back(button_spec);
				button_groups[group_index].edge_offset = edge_offset == 0.f ? button_groups[group_index].edge_offset : edge_offset;
				button_groups[group_index].align = align == GroupAlign::Null ? button_groups[group_index].align : align;
				button_groups[group_index].inner_spacing = spacing == 0 ? button_groups[group_index].inner_spacing : spacing;
			}
		}

		// Define DRAW_CT_EXCLUSIONS with the default titlebar drawing function to see the exclusion rects
		void AddExclusion(const ExclusionSpec& exclusion)
		{
			exclusions.push_back(exclusion);
		}

		int height = 0;

		int top_border = TopBorder::Default;

		std::array<ButtonGroup,3> button_groups;

		std::vector<ExclusionSpec> exclusions;

		void (*titlebar_draw_f)(const AppProperties*, GLFWwindow*) = nullptr;
		void (*titlebar_scaling_f)(Titlebar_Properties*, float, GLFWwindow*) = nullptr;
	};

	void End();
	bool Begin(const char* name, bool* p_open = NULL, ImGuiWindowFlags flags = 0);

	//Use this function if DPI awerness is enabled
	//     ImGui::PushFont(GetFont(FONT_TYPE));
	//Returns the font scaled to the current window's dpi
	//type is the font type, see the FONT_ defines
	ImFont* GetFont(int type);

	void DefCustomTitlebarDraw(const AppProperties* app_props,GLFWwindow* window);
	void DefCustomTitlebarScaling(Titlebar_Properties* out_props, float scale, GLFWwindow* window);

	struct AppProperties
	{
		const char* name;
		const char* ini_file = nullptr;

		union 
		{
			float relative;
			int abosulte;
		} width{ .abosulte = 0 };

		union 
		{
			float relative;
			int abosulte;
		} height{ .abosulte = 0 };

		bool relative_size = true;

		int min_width = 0;
		int min_height = 0;
		
		float font_size = 18;
		IconFont font_icon = IconFont::FontAwesome6_Solid;
		float icon_size = 0.f;

		bool dpi_aware = true;
		bool resizeable = true;
		bool fullscreen = false;
		bool maximized = false;
		bool imgui_docking = false;
		bool center_window = true;
		bool gen_ini = false;

		Titlebar_Properties custom_titlebar_props;
		bool custom_titlebar = false;

		AppProperties() : name("My Application") {};
	};

	class Application
	{
	public:
		explicit Application(const AppProperties& props = AppProperties());

		//Will run once before the window rendering but after the backend initalization
		void SetUp(std::function<void()> func);

		void Run();
		void Exit();

		static void SetPrurpleDarkColorTheme();
		static void SetDarkColorTheme();

		static void SetDefaultProperties(AppProperties& props);

		void SetTitlebarProperties(const Titlebar_Properties& props);

		void AddPanel(std::shared_ptr<Panel> panel);
		void RemovePanel(uint64_t id);

		//No need for Begin/End just DockBuilderCode
		//Arguments : ImGuiID of the dockspace and a bool reference to the firstrun var, which if false will not run the code again
		//Firstrun should always be set to false, unless multiple setups are needed, in which case the first run should be checked inside the function
		void AddDefDockingSetup(std::function<void(ImGuiID,bool&)> setup_func);
		ImGuiID GetDockspaceId() const;

		void __CallScaleCallback(float x, float y);

		friend void End();
		friend bool Begin(const char* name, bool* p_open, ImGuiWindowFlags flags);

		//Use this function if DPI awerness is enabled
		// ImGui::PushFont(GetFont(FONT_TYPE));
		//Returns the font scaled to the current window's dpi
		//type is the font type, see the FONT_ defines
		friend ImFont* GetFont(int type);

	private:

		struct SizeRect
		{
			int width;
			int height;

			SizeRect(int width, int height) : width(width), height(height) {};
			SizeRect() : width(0), height(0) {};

			bool operator!=(const SizeRect& other)
			{
				return width != other.width || height != other.height;
			}
		};

		bool Init();
		bool CreateApplication();
		void Shutdown(bool failure);

		void BeginRender();
		void EndRender();

		void BeginRenderDockspace();

		void UpdateGLFWTitlebarRects();

		IMAF::Application::SizeRect GetMainApplicationScreenSize();

	private:
		GLFWwindow* mp_window = nullptr;
		AppProperties m_props;
		ImGuiID m_dockspace_id = 0;

		bool m_should_exit = false;

		std::mutex m_panels_mutex;
		std::vector<std::shared_ptr<Panel>> m_panels;

		std::function<void()> mp_setup_func;
		std::function<void(ImGuiID,bool&)> mp_def_docking;
		
		Scale::Scaler* mp_scaler = nullptr;

		SizeRect m_screen_size;
		float m_dpi_scale = 1.0f;

		std::unordered_map<float,std::vector<ImFont*>> m_fonts;
	};

}