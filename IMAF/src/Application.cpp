#include "Application.h"

#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "GLES3/gl3.h"
#include "GLFW/glfw3.h"

#include <Windows.h>

#include "fonts.h"

#define ValidTitlebarArea(area) (area >= 0.f && area <= 1.f)
#include <string>
static void glfw_error_callback(int error, const char* description)
{
	
}

namespace IMAF 
{
	
	Application::Application(const AppProperties& props) : m_props(props) 
	{
		if (Init())
		{
			MessageBoxA(NULL, "Application Initalization Failed", "Fatal Error", MB_OK | MB_ICONERROR);
			Shutdown();
			ExitProcess(1);
		}
	}

	Application::~Application()
	{
		Shutdown();
	}

	bool Application::Init()
	{
		glfwSetErrorCallback(glfw_error_callback);
		if (!glfwInit())
			return true;


		glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
		glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

		if (m_props.resizeable)
			glfwWindowHint(GLFW_RESIZABLE, true);
		else
			glfwWindowHint(GLFW_RESIZABLE, false);

		GLFWmonitor* monitor = nullptr;
		if (m_props.fullscreen) 
			monitor = glfwGetPrimaryMonitor();

		if (m_props.maximized)
		{
			int screen_width = GetSystemMetrics(SM_CXSCREEN);
			int screen_height = GetSystemMetrics(SM_CYSCREEN);
			m_props.width = screen_width;
			m_props.height = screen_height;
		}

		if (m_props.width == 0 || m_props.height == 0)
		{
			float screen_width = GetSystemMetrics(SM_CXSCREEN);
			float screen_height = GetSystemMetrics(SM_CYSCREEN);
			m_props.width = screen_width * 0.75;
			m_props.height = screen_height * 0.85;
		}

		if (m_props.costum_titlebar && ValidTitlebarArea(m_props.costum_titlebar_area))
			glfwWindowHint(GLFW_TITLEBAR, true);

		// Create window with graphics context
		mp_window = glfwCreateWindow(m_props.width, m_props.height, m_props.name, monitor, NULL);
		if (mp_window == nullptr)
			return true;

		glfwMakeContextCurrent(mp_window);
		glfwSwapInterval(1);

		if (glfwGetWindowAttrib(mp_window, GLFW_TITLEBAR))
		{
			glfwSetWindowAttrib(mp_window, GLFW_TITLEBAR_AREA, (m_props.costum_titlebar_area * 100.f));
		}

		if (m_props.center_window && !m_props.fullscreen) 
		{
			int screen_width = GetSystemMetrics(SM_CXSCREEN);
			int screen_height = GetSystemMetrics(SM_CYSCREEN);
			
			int pos_x = screen_width / 2 - m_props.width / 2;
			int pos_y = screen_height / 2 - m_props.height / 2;

			glfwSetWindowPos(mp_window, pos_x, pos_y);
		}

		glfwShowWindow(mp_window);

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;

		if (m_props.imgui_docking)
			io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
		//io.ConfigViewportsNoAutoMerge = true;
		//io.ConfigViewportsNoTaskBarIcon = true;

		Application::SetPrurpleDarkColorTheme();

		// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}
		style.FrameRounding = 6.0f;
		style.WindowBorderSize = 0.0f;
		style.FramePadding = { 8.0f,6.0f };
		style.GrabRounding = 6.0f;
		style.WindowPadding = { 4.f,4.f };

		// Setup Platform/Renderer backends
		ImGui_ImplGlfw_InitForOpenGL(mp_window, true);
		ImGui_ImplOpenGL3_Init("#version 460");

		//Adding default font
		ImFontConfig fontConfig;
		fontConfig.FontDataOwnedByAtlas = false;
		io.Fonts->AddFontFromMemoryTTF((void*)&g_FontRegular[0], sizeof(g_FontRegular), m_props.font_size, &fontConfig);
		io.Fonts->AddFontFromMemoryTTF((void*)&g_FontLight[0], sizeof(g_FontLight), m_props.font_size, &fontConfig);
		io.Fonts->AddFontFromMemoryTTF((void*)&g_FontMedium[0], sizeof(g_FontMedium), m_props.font_size, &fontConfig);
		io.Fonts->AddFontFromMemoryTTF((void*)&g_FontSemibold[0], sizeof(g_FontSemibold), m_props.font_size, &fontConfig);
		io.Fonts->AddFontFromMemoryTTF((void*)& g_FontBold[0], sizeof(g_FontBold), m_props.font_size, &fontConfig);
		io.Fonts->AddFontFromMemoryTTF((void*)&g_FontExtrabold[0], sizeof(g_FontExtrabold), m_props.font_size, &fontConfig);

		return false;
	}

	void Application::AddPanel(std::shared_ptr<Panel> panel)
	{
		if (panel != nullptr)
		{
			m_panels_mutex.lock();
			m_panels.push_back(panel);
			m_panels_mutex.unlock();
		}
	}

	void Application::Run()
	{
		if (mp_setup_func)
			mp_setup_func();

		while (!glfwWindowShouldClose(mp_window) && !m_should_exit)
		{
			BeginRender();

			if (m_props.imgui_docking)
				BeginRenderDockspace();

			m_panels_mutex.lock();
			for (auto& i : m_panels)
			{
				i->UiRender();
			}
			m_panels_mutex.unlock();

			//End dockspace window
			if (m_props.imgui_docking)
				ImGui::End();
			EndRender();
		}
	}

	void Application::Shutdown()
	{
		// Cleanup
		if (ImGui::GetIO().BackendRendererUserData != nullptr)
			ImGui_ImplOpenGL3_Shutdown();
		if (ImGui::GetIO().BackendPlatformUserData != nullptr)
			ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();

		if (glfwInit())
			glfwDestroyWindow(mp_window);
		glfwTerminate();
	}

	void Application::BeginRender()
	{
		glfwPollEvents();

		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
	}

	void Application::EndRender()
	{
		constexpr ImVec4 clear_color = RGB2_IMVEC4(255, 255, 255);

		ImGui::Render();

		ImDrawData* main_draw_data = ImGui::GetDrawData();
		const bool minimized = (main_draw_data->DisplaySize.x <= 0.0f || main_draw_data->DisplaySize.y <= 0.0f);
		
		if (!minimized)
		{
			int display_w, display_h;
			glfwGetFramebufferSize(mp_window, &display_w, &display_h);
			glViewport(0, 0, display_w, display_h);
			glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
			glClear(GL_COLOR_BUFFER_BIT);
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		}

		// Update and Render additional Platform Windows
		// (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
		//  For this specific demo app we could also call glfwMakeContextCurrent(window) directly)
		if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			GLFWwindow* backup_current_context = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			glfwMakeContextCurrent(backup_current_context);
		}

		if (!minimized)
			glfwSwapBuffers(mp_window);
		else
			Sleep(5);
	}

	void Application::SetDefaultProperties(AppProperties& props)
	{
		props.costum_titlebar = false;
		props.font_size = 18;
		props.fullscreen = false;
		props.height = 500;
		props.width = 720;
		props.resizeable = true;
		props.name = "My Application";
		props.imgui_docking = false;
		props.costum_titlebar_area = 0.f;
	}

	void Application::Exit()
	{
		m_should_exit = true;
	}

	void Application::BeginRenderDockspace()
	{
		int width = 0, height = 0;
		int pos_x = 0, pos_y = 0;
		glfwGetWindowSize(mp_window, &width, &height);
		glfwGetWindowPos(mp_window, &pos_x, &pos_y);

		ImVec2 window_size = { (float)width,(float)height };

		ImGui::SetNextWindowSize(window_size);
		ImGui::SetNextWindowPos({ (float)pos_x,(float)pos_y });

		ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize
			| ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoDocking;

		if (!m_props.gen_ini)
			flags |= ImGuiWindowFlags_NoSavedSettings;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
		ImGui::PushStyleColor(ImGuiCol_::ImGuiCol_WindowBg, ImVec4(0.09f, 0.09f, 0.10f, 1.00f));
		ImGui::Begin("DockingWindow", nullptr, flags);

		ImGui::DockSpace(ImGui::GetID("Dockspace"));

		ImGui::PopStyleVar();
		ImGui::PopStyleColor();
	}

	void Application::RemovePanel(uint64_t id)
	{
		m_panels_mutex.lock();
		std::erase_if(m_panels, [id](std::shared_ptr<IMAF::Panel> ptr) -> bool { return ptr->GetId() == id ? true : false; });
		m_panels_mutex.unlock();
	}

	void Application::SetPrurpleDarkColorTheme()
	{
		ImVec4* colors = ImGui::GetStyle().Colors;
		colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
		colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
		colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.11f, 1.00f);
		colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
		colors[ImGuiCol_Border] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
		colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.20f, 0.21f, 1.00f);
		colors[ImGuiCol_FrameBgHovered] = ImVec4(0.30f, 0.31f, 0.31f, 1.00f);
		colors[ImGuiCol_FrameBgActive] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
		colors[ImGuiCol_TitleBg] = ImVec4(0.24f, 0.13f, 0.36f, 1.00f);
		colors[ImGuiCol_TitleBgActive] = ImVec4(0.45f, 0.25f, 0.58f, 1.00f);
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
		colors[ImGuiCol_MenuBarBg] = ImVec4(0.56f, 0.34f, 0.78f, 1.00f);
		colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
		colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
		colors[ImGuiCol_CheckMark] = ImVec4(0.56f, 0.34f, 0.78f, 1.00f);
		colors[ImGuiCol_SliderGrab] = ImVec4(0.56f, 0.34f, 0.78f, 1.00f);
		colors[ImGuiCol_SliderGrabActive] = ImVec4(0.47f, 0.29f, 0.66f, 1.00f);
		colors[ImGuiCol_Button] = ImVec4(0.20f, 0.20f, 0.21f, 1.00f);
		colors[ImGuiCol_ButtonHovered] = ImVec4(0.31f, 0.13f, 0.50f, 1.00f);
		colors[ImGuiCol_ButtonActive] = ImVec4(0.27f, 0.12f, 0.44f, 1.00f);
		colors[ImGuiCol_Header] = ImVec4(0.41f, 0.17f, 0.66f, 1.00f);
		colors[ImGuiCol_HeaderHovered] = ImVec4(0.64f, 0.39f, 0.89f, 1.00f);
		colors[ImGuiCol_HeaderActive] = ImVec4(0.46f, 0.27f, 0.65f, 1.00f);
		colors[ImGuiCol_Separator] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
		colors[ImGuiCol_SeparatorHovered] = ImVec4(0.55f, 0.27f, 0.73f, 199.00f);
		colors[ImGuiCol_SeparatorActive] = ImVec4(0.44f, 0.23f, 0.66f, 1.00f);
		colors[ImGuiCol_ResizeGrip] = ImVec4(0.82f, 0.82f, 0.82f, 90.00f);
		colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.71f, 0.71f, 0.71f, 170.00f);
		colors[ImGuiCol_ResizeGripActive] = ImVec4(0.30f, 0.30f, 0.30f, 240.00f);
		colors[ImGuiCol_Tab] = ImVec4(0.43f, 0.26f, 0.60f, 1.00f);
		colors[ImGuiCol_TabHovered] = ImVec4(0.56f, 0.34f, 0.78f, 1.00f);
		colors[ImGuiCol_TabActive] = ImVec4(0.73f, 0.48f, 0.91f, 1.00f);
		colors[ImGuiCol_TabUnfocused] = ImVec4(0.24f, 0.13f, 0.36f, 1.00f);
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.33f, 0.20f, 0.44f, 1.00f);
		colors[ImGuiCol_DockingPreview] = ImVec4(0.66f, 0.40f, 0.95f, 0.66f);
		colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
		colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
		colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.84f, 0.46f, 1.00f, 1.00f);
		colors[ImGuiCol_PlotHistogram] = ImVec4(0.56f, 0.34f, 0.78f, 1.00f);
		colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.84f, 0.46f, 1.00f, 1.00f);
		colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
		colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
		colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
		colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
		colors[ImGuiCol_TextSelectedBg] = ImVec4(0.65f, 0.39f, 0.85f, 0.39f);
		colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
		colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
		colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
		colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
	}

	void Application::SetDarkColorTheme()
	{
		ImVec4* colors = ImGui::GetStyle().Colors;
		colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
		colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
		colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.11f, 1.00f);
		colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
		colors[ImGuiCol_Border] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
		colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.20f, 0.21f, 1.00f);
		colors[ImGuiCol_FrameBgHovered] = ImVec4(0.30f, 0.31f, 0.31f, 1.00f);
		colors[ImGuiCol_FrameBgActive] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
		colors[ImGuiCol_TitleBg] = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
		colors[ImGuiCol_TitleBgActive] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
		colors[ImGuiCol_MenuBarBg] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
		colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
		colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
		colors[ImGuiCol_CheckMark] = ImVec4(0.76f, 0.76f, 0.76f, 1.00f);
		colors[ImGuiCol_SliderGrab] = ImVec4(0.46f, 0.46f, 0.46f, 1.00f);
		colors[ImGuiCol_SliderGrabActive] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
		colors[ImGuiCol_Button] = ImVec4(0.20f, 0.20f, 0.21f, 1.00f);
		colors[ImGuiCol_ButtonHovered] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
		colors[ImGuiCol_ButtonActive] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
		colors[ImGuiCol_Header] = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
		colors[ImGuiCol_HeaderHovered] = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
		colors[ImGuiCol_HeaderActive] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
		colors[ImGuiCol_Separator] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
		colors[ImGuiCol_SeparatorHovered] = RGBA2_IMVEC4(123, 123, 123, 199);
		colors[ImGuiCol_Tab] = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
		colors[ImGuiCol_TabHovered] = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
		colors[ImGuiCol_TabActive] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
		colors[ImGuiCol_TabUnfocused] = ImVec4(0.07f, 0.10f, 0.15f, 0.97f);
		colors[ImGuiCol_TabUnfocusedActive] = RGB2_IMVEC4(46, 46, 46);
		colors[ImGuiCol_DockingPreview] = ImVec4(0.66f, 0.66f, 0.66f, 0.66f);
		colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
		colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
		colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
		colors[ImGuiCol_PlotHistogram] = ImVec4(0.74f, 0.74f, 0.74f, 1.00f);
		colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
		colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
		colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
		colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
		colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
		colors[ImGuiCol_TextSelectedBg] = ImVec4(0.78f, 0.78f, 0.78f, 0.35f);
		colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
		colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
		colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
		colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
		colors[ImGuiCol_SeparatorActive] = RGB2_IMVEC4(70, 70, 70);
		colors[ImGuiCol_ResizeGrip] = RGBA2_IMVEC4(210, 210, 210, 90);
		colors[ImGuiCol_ResizeGripActive] = RGBA2_IMVEC4(76, 76, 76, 240);
		colors[ImGuiCol_ResizeGripHovered] = RGBA2_IMVEC4(180, 180, 180, 170);
	}
}