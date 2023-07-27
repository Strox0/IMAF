#pragma once

#include "imgui.h"

#include <memory>
#include <functional>

#include "Panel.h"

#define RGB2_IMVEC4(r,g,b) ImVec4{ r / 255.0f , g / 255.0f ,b / 255.0f, 1.0f }
#define RGBA2_IMVEC4(r,g,b,a) ImVec4{ r / 255.0f , g / 255.0f ,b / 255.0f, a }

#define FONT_DEFAULT				0
#define FONT_NORMAL					0
#define FONT_LIGHT_NORMAL			1
#define FONT_MEDIUM_NORMAL			2
#define FONT_SEMIBOLD_NORMAL		3
#define FONT_BOLD_NORMAL			4
#define FONT_EXTRABOLD_NORMAL		5

struct GLFWwindow;

namespace IMAF 
{
	struct AppProperties
	{
		const char* name;
		
		int width = 720;
		int height = 500;
		float font_size = 18;

		bool costum_titlebar = false;
		bool resizeable = true;
		bool fullscreen = false;
		bool imgui_docking = false;
		bool center_window = true;

		//Reletive area startig from the top of the application
		//1.0 = whole app is titlebar
		//0.1 = 10% of the app is titlebar
		float costum_titlebar_area = 0.f;

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

		static void SetPrurpleDarkColorTheme();
		static void SetDarkColorTheme();

		static void SetDefaultProperties(AppProperties& props);

		void AddPanel(std::shared_ptr<Panel> panel);

	private:
		bool Init();
		void Shutdown();

		void BeginRender();
		void EndRender();

	private:
		GLFWwindow* mp_window = nullptr;
		AppProperties m_props;

		std::vector<std::shared_ptr<Panel>> m_panels;

		std::function<void()> mp_setup_func;

	};

}