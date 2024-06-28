#pragma once

#include "imgui.h"

#include <memory>
#include <functional>
#include <mutex>
#include <Windows.h>

#include "Panel.h"
#include "Scale.h"

#define RGB2_IMVEC4(r,g,b) ImVec4( r / 255.0f , g / 255.0f ,b / 255.0f, 1.0f )
#define RGBA2_IMVEC4(r,g,b,a) ImVec4( r / 255.0f , g / 255.0f ,b / 255.0f, a )

#define FONT_DEFAULT				0
#define FONT_NORMAL					0
#define FONT_LIGHT_NORMAL			1
#define FONT_MEDIUM_NORMAL			2
#define FONT_SEMIBOLD_NORMAL		3
#define FONT_BOLD_NORMAL			4
#define FONT_EXTRABOLD_NORMAL		5
#define FONT_LAST FONT_EXTRABOLD_NORMAL

struct GLFWwindow;

namespace IMAF 
{
	struct ButtonSpec
	{
		void SetRect(int x, int height)
		{
			width = height;
			this->x = x;
			y = 0;
			height = -1;
		}

		void SetRect(int x, int y, int width, int height)
		{
			this->x = x;
			this->width = width;
			this->height = height;
			this->y = y;
		}

		int x = 0;
		int y = 0;
		int width = 50;
		int height = -1;
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
		int height = 0;

		int top_border = TopBorder::Default;

		ButtonSpec close_button;
		ButtonSpec minimize_button;
		ButtonSpec maximize_button;

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

		int width = 0;
		int height = 0;
		int min_width = 0;
		int min_height = 0;
		float font_size = 18;
		
		bool dpi_aware = true;
		bool resizeable = true;
		bool fullscreen = false;
		bool maximized = false;
		bool imgui_docking = false;
		bool center_window = true;
		bool gen_ini = false;

		//WIP
		Titlebar_Properties custom_titlebar_props;
		bool custom_titlebar = false;

		AppProperties() : name("My Application") {};
	};

	class Application
	{
	public:
		explicit Application(const AppProperties& props = AppProperties());
		~Application();

		//Will run once before the window rendering but after the backend initalization
		void SetUp(std::function<void()> func);

		void Run();
		void Exit();

		static void SetPrurpleDarkColorTheme();
		static void SetDarkColorTheme();

		static void SetDefaultProperties(AppProperties& props);

		void AddPanel(std::shared_ptr<Panel> panel);
		void RemovePanel(uint64_t id);

		//No need for Begin/End just DockBuilderCode
		//Arguments : ImGuiID of the dockspace and a bool reference to the firstrun var, which if false will not run the code again
		//Firstrun should always be set to false, unless multiple setups are needed, in which case the first run should be checked inside the function
		void AddDefDockingSetup(std::function<void(ImGuiID,bool&)> setup_func);
		ImGuiID GetDockspaceId() const;

		void __CallScaleCallback(float x, float y);

		void ReCaclWindowSize();

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
		void Shutdown();

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
		bool m_exited = true;

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