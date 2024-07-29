#define IMGUI_DEFINE_MATH_OPERATORS
#include "IMAF/Application.h"

#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "imgui_internal.h"

#include "GLES3/gl3.h"

#define GLFW_EXPOSE_NATIVE_WIN32
#include "GLFW/glfw3native.h"

#include "IMAF/fonts.h"

#include <string>

IMAF::Application* g_app = nullptr;

static void glfw_error_callback(int error, const char* description) {}

void __DPICallBack(GLFWwindow* window, float xscale, float yscale)
{
	IMAF::Application* app = (IMAF::Application*)glfwGetWindowUserPointer(window);
	app->__CallScaleCallback(xscale, yscale);
}

namespace IMAF
{
	Application::Application(const AppProperties& props) : m_props(props) 
	{
		if (!Init())
		{
			MessageBoxA(NULL, "Application Initalization Failed", "Fatal Error", MB_OK | MB_ICONERROR);
			Shutdown(true);
			ExitProcess(1);
		}

		g_app = this;

		if (m_props.dpi_aware)
		{
			Scale::InitScaler();
			mp_scaler = Scale::GetScaler();
		}
	}

	bool Application::Init()
	{
		glfwSetErrorCallback(glfw_error_callback);
		if (!glfwInit())
			return false;

		glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
		glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

		if (m_props.dpi_aware)
			glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);

		if (m_props.resizeable)
			glfwWindowHint(GLFW_RESIZABLE, true);
		else
			glfwWindowHint(GLFW_RESIZABLE, false);

		if (m_props.relative_size)
		{
			if (m_props.width.relative == 0 || m_props.width.relative > 1)
				m_props.width.relative = 0.65f;

			if (m_props.height.relative == 0 || m_props.height.relative > 1)
				m_props.height.relative = 0.75f;

			int screen_width = GetSystemMetrics(SM_CXSCREEN);
			int screen_height = GetSystemMetrics(SM_CYSCREEN);
			m_props.width.abosulte = screen_width * m_props.width.relative;
			m_props.height.abosulte = screen_height * m_props.height.relative;
		}
		else if (m_props.width.abosulte <= 0 || m_props.height.abosulte <= 0)
		{
			int screen_width = GetSystemMetrics(SM_CXSCREEN);
			int screen_height = GetSystemMetrics(SM_CYSCREEN);
			m_props.width.abosulte = screen_width * 0.65f;
			m_props.height.abosulte = screen_height * 0.75f;
		}
		
		if (m_props.custom_titlebar)
		{
			switch (m_props.custom_titlebar_props.top_border)
			{
			case TopBorder::Color:
				glfwWindowHint(GLFW_CT_TOP_BORDER, GLFW_CT_TOP_COLOR_CHANGE);
				break;
			case TopBorder::Thin:
				glfwWindowHint(GLFW_CT_TOP_BORDER, GLFW_CT_TOP_THIN_BORDER);
				break;
			case TopBorder::None:
				glfwWindowHint(GLFW_CT_TOP_BORDER, GLFW_CT_TOP_NO_BORDER);
				break;
			default:
				glfwWindowHint(GLFW_CT_TOP_BORDER, GLFW_CT_TOP_DEFAULT_BORDER);
				break;
			}
		}

		return true;
	}

	bool Application::CreateApplication()
	{
		GLFWmonitor* monitor = nullptr;
		if (m_props.fullscreen)
			monitor = glfwGetPrimaryMonitor();

		mp_window = glfwCreateWindow(m_props.width.abosulte, m_props.height.abosulte, m_props.name, monitor, NULL); //Create Window
		if (mp_window == nullptr)
			return false;

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;

		if (m_props.imgui_docking)
			io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
		//io.ConfigViewportsNoAutoMerge = true;
		//io.ConfigViewportsNoTaskBarIcon = true;

		if (m_props.dpi_aware)
		{
			glfwSetWindowSize(mp_window, m_props.width.abosulte, m_props.height.abosulte);
			glfwSetWindowContentScaleCallback(mp_window, __DPICallBack);
		}

		m_screen_rect = GetMainApplicationScreen();
		SizeRect size_calc_screen(GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));

		if (m_screen_rect.GetSize() != size_calc_screen)
		{
			m_props.width.abosulte = m_screen_rect.GetSize().width * m_props.width.relative;
			m_props.height.abosulte = m_screen_rect.GetSize().height * m_props.height.relative;
			glfwSetWindowSize(mp_window, m_props.width.abosulte, m_props.height.abosulte);
		}

		glfwSetWindowUserPointer(mp_window, this);
		glfwMakeContextCurrent(mp_window);
		glfwSwapInterval(1);

		if (m_props.custom_titlebar)
		{
			UpdateGLFWTitlebarRects();

			glfwSetCustomTitleBar(mp_window, true);

			if (m_props.custom_titlebar_props.titlebar_draw_f == nullptr)
				m_props.custom_titlebar_props.titlebar_draw_f = DefCustomTitlebarDraw;

			if (m_props.custom_titlebar_props.titlebar_scaling_f == nullptr)
				m_props.custom_titlebar_props.titlebar_scaling_f = DefCustomTitlebarScaling;
		}

		if (m_props.min_height != 0 && m_props.min_width != 0 && m_props.resizeable)
		{
			glfwSetWindowSizeLimits(mp_window, m_props.min_width, m_props.min_height, GLFW_DONT_CARE, GLFW_DONT_CARE);
		}

		if (m_props.maximized)
			glfwMaximizeWindow(mp_window);

		if (m_props.center_window && !m_props.fullscreen)
		{
			int w = std::abs(m_screen_rect.right - m_screen_rect.left);
			int h = std::abs(m_screen_rect.bottom - m_screen_rect.top);

			int center_x = m_screen_rect.left + w / 2;
			int center_y = m_screen_rect.top + h / 2;

			int pos_x = center_x - m_props.width.abosulte / 2;
			int pos_y = center_y - m_props.height.abosulte / 2;

			glfwSetWindowPos(mp_window, pos_x, pos_y);
		}

		if (m_props.ini_file)
			io.IniFilename = m_props.ini_file;

		if (!m_props.gen_ini)
			io.IniFilename = NULL;

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
		style.WindowPadding = { 10.f,4.f };

		if (!m_props.custom_titlebar)
			style.Colors[ImGuiCol_DockingEmptyBg] = RGBA2_IMVEC4(0, 0, 0, 0);

		// Setup Platform/Renderer backends
		ImGui_ImplGlfw_InitForOpenGL(mp_window, true);
		ImGui_ImplOpenGL3_Init("#version 460");

		//Load fonts
		ImFontConfig fontConfig;
		fontConfig.FontDataOwnedByAtlas = false;

		ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();

		if (m_props.icon_size == 0.f)
			m_props.icon_size = m_props.font_size * 2;

		for (const auto& i : platform_io.Monitors)
		{
			if (!m_props.dpi_aware && i.MainSize.x != m_screen_rect.GetSize().width && i.MainSize.y != m_screen_rect.GetSize().height)
				continue;

			std::vector<ImFont*> fonts;
			float size = std::floorf((float)m_props.font_size * i.DpiScale);
			fonts.push_back(io.Fonts->AddFontFromMemoryTTF((void*)&g_FontRegular[0], sizeof(g_FontRegular), size, &fontConfig));

			static const ImWchar icon_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
			fontConfig.MergeMode = true;
			fontConfig.PixelSnapH = true;
			fontConfig.GlyphMinAdvanceX = size;

			if (m_props.font_icon == IconFont::FontAwesome6_Regluar)
				fonts.push_back(io.Fonts->AddFontFromMemoryTTF((void*)&g_IconsRegular[0], sizeof(g_IconsRegular), size, &fontConfig, icon_ranges));
			else if (m_props.font_icon == IconFont::FontAwesome6_Solid)
				fonts.push_back(io.Fonts->AddFontFromMemoryTTF((void*)&g_IconsSolid[0], sizeof(g_IconsSolid), size, &fontConfig, icon_ranges));

			fontConfig.MergeMode = false;

			if (m_props.font_icon == IconFont::FontAwesome6_Regluar)
				fonts.push_back(io.Fonts->AddFontFromMemoryTTF((void*)&g_IconsRegular[0], sizeof(g_IconsRegular), std::floorf(m_props.icon_size * i.DpiScale), &fontConfig, icon_ranges));
			else if (m_props.font_icon == IconFont::FontAwesome6_Solid)
				fonts.push_back(io.Fonts->AddFontFromMemoryTTF((void*)&g_IconsSolid[0], sizeof(g_IconsSolid), std::floorf(m_props.icon_size * i.DpiScale), &fontConfig, icon_ranges));

			fontConfig.PixelSnapH = false;
			fontConfig.GlyphMinAdvanceX = 0;

			fonts.push_back(io.Fonts->AddFontFromMemoryTTF((void*)&g_FontLight[0], sizeof(g_FontLight), size, &fontConfig));
			fonts.push_back(io.Fonts->AddFontFromMemoryTTF((void*)&g_FontMedium[0], sizeof(g_FontMedium), size, &fontConfig));
			fonts.push_back(io.Fonts->AddFontFromMemoryTTF((void*)&g_FontSemibold[0], sizeof(g_FontSemibold), size, &fontConfig));
			fonts.push_back(io.Fonts->AddFontFromMemoryTTF((void*)&g_FontBold[0], sizeof(g_FontBold), size, &fontConfig));
			fonts.push_back(io.Fonts->AddFontFromMemoryTTF((void*)&g_FontExtrabold[0], sizeof(g_FontExtrabold), size, &fontConfig));

			m_fonts[i.DpiScale] = fonts;
		}

		float xscale, yscale;
		glfwGetWindowContentScale(mp_window, &xscale, &yscale);
		io.FontDefault = m_fonts[xscale][0];
		style.ScaleAllSizes(xscale);
		m_dpi_scale = xscale;

		if (mp_scaler)
			mp_scaler->SetMainWindowScale((xscale < yscale ? xscale : yscale));
		
		if (m_props.dpi_aware && !m_props.relative_size)
		{
			glfwSetWindowSize(mp_window, m_props.width.abosulte * xscale, m_props.height.abosulte * yscale);
		}

		glfwShowWindow(mp_window);

		return true;
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

	void Application::SetUp(std::function<void(const IMAF::AppProperties&,GLFWwindow*)> func)
	{
		mp_setup_func = func;
	}

	void Application::Run()
	{
		if (!CreateApplication())
		{
			MessageBoxA(NULL, "Application Creation Failed", "Fatal Error", MB_OK | MB_ICONERROR);
			Shutdown(true);
			ExitProcess(1);
		}

		if (mp_setup_func)
			mp_setup_func(m_props,mp_window);

		while (!glfwWindowShouldClose(mp_window) && !m_should_exit)
		{
			BeginRender();

			if (m_props.custom_titlebar)
				m_props.custom_titlebar_props.titlebar_draw_f(&m_props,mp_window);

			if (m_props.imgui_docking)
				BeginRenderDockspace();

			m_panels_mutex.lock();
			std::vector<std::shared_ptr<Panel>> panels_copy = m_panels;
			m_panels_mutex.unlock();

			for (auto& i : panels_copy)
			{
				i->UiRender();
			}

			//End dockspace window
			if (m_props.imgui_docking)
			{
				static bool first_run = true;
				if (mp_def_docking && first_run)
				{
					mp_def_docking(m_dockspace_id,first_run);
				}
				IMAF::End();
			}

			if (mp_scaler)
				mp_scaler->UpdateWindowScales();

			EndRender();
		}

		Shutdown(false);
	}

	void Application::Shutdown(bool failure)
	{
		if (mp_scaler)
			mp_scaler->Shutdown();

		if (!failure)
		{
			if (ImGui::GetIO().BackendRendererUserData != nullptr)
				ImGui_ImplOpenGL3_Shutdown();
			if (ImGui::GetIO().BackendPlatformUserData != nullptr)
				ImGui_ImplGlfw_Shutdown();
			ImGui::DestroyContext();

			glfwDestroyWindow(mp_window);
			glfwTerminate();
		}
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
		props.custom_titlebar = false;
		props.font_size = 18;
		props.fullscreen = false;
		props.height.relative = 0.65;
		props.width.relative = 0.75;
		props.resizeable = true;
		props.name = "My Application";
		props.imgui_docking = false;
		props.gen_ini = false;
		props.center_window = true;
		props.maximized = false;
		props.dpi_aware = true;	
	}

	void Application::SetTitlebarProperties(const Titlebar_Properties& props)
	{
		m_props.custom_titlebar_props = props;

		if (m_props.custom_titlebar_props.titlebar_draw_f == nullptr)
			m_props.custom_titlebar_props.titlebar_draw_f = DefCustomTitlebarDraw;

		if (m_props.custom_titlebar_props.titlebar_scaling_f == nullptr)
			m_props.custom_titlebar_props.titlebar_scaling_f = DefCustomTitlebarScaling;

		UpdateGLFWTitlebarRects();
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
		ImVec2 window_pos = { (float)pos_x,(float)pos_y };

		if (m_props.custom_titlebar)
		{
			window_size.y -= m_props.custom_titlebar_props.height;
			window_pos.y += m_props.custom_titlebar_props.height;
		}

		ImGui::SetNextWindowSize(window_size);
		ImGui::SetNextWindowPos(window_pos);

		ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize
			| ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoDocking;

		if (m_props.gen_ini)
			flags |= ImGuiWindowFlags_NoSavedSettings;
		
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.f,0.f });
		
		ImGui::PushStyleColor(ImGuiCol_::ImGuiCol_WindowBg, ImVec4(0.09f, 0.09f, 0.10f, 1.00f));

		IMAF::Begin("DockingWindow", nullptr, flags);

		if (m_dockspace_id == 0)
			m_dockspace_id = ImGui::GetID("MyDockspace");

		ImGui::DockSpace(m_dockspace_id);

		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor();
	}

	void Application::UpdateGLFWTitlebarRects()
	{
		glfwSetCustomTitlebarHeight(mp_window, m_props.custom_titlebar_props.height);
		
		GLFWChainSpec* chain = nullptr;
		GLFWChainSpec* start = nullptr;
		for (const auto& i : m_props.custom_titlebar_props.exclusions)
		{
			if (chain == nullptr)
			{
				chain = new GLFWChainSpec();
				start = chain;
			}
			else
			{
				chain->next = new GLFWChainSpec();
				chain = chain->next;
			}

			chain->height = i.height;
			chain->width = i.width;
			chain->topOffset = i.top_offset;
			chain->startOffset = i.start_offset;
		}
		
		if (start)
		{
			glfCustomTitlebarRemoveExclusions(mp_window);
			glfwCustomTitlebarAddExclusion(mp_window, start);
		}

		for (int i = 0; i < 3; i++)
		{
			if (m_props.custom_titlebar_props.button_groups[i].buttons.size() == 0)
				continue;

			GLFWChainSpec* chain = new GLFWChainSpec();
			GLFWChainSpec* start = chain;
			for (size_t j = 0; j < m_props.custom_titlebar_props.button_groups[i].buttons.size(); j++)
			{
				if (m_props.custom_titlebar_props.button_groups[i].buttons[j].height == -1)
					m_props.custom_titlebar_props.button_groups[i].buttons[j].height = m_props.custom_titlebar_props.height;

				chain->height = m_props.custom_titlebar_props.button_groups[i].buttons[j].height;
				chain->width = m_props.custom_titlebar_props.button_groups[i].buttons[j].width;
				chain->buttonType = m_props.custom_titlebar_props.button_groups[i].buttons[j].type;
				
				if (j + 1 != m_props.custom_titlebar_props.button_groups[i].buttons.size())
				{
					chain->next = new GLFWChainSpec();
					chain = chain->next;
				}
			}

			glfwCustomTitlebarSetGroupAlignment(mp_window, i, m_props.custom_titlebar_props.button_groups[i].align);
			glfwCustomTitlebarSetGroupOffset(mp_window, i, m_props.custom_titlebar_props.button_groups[i].edge_offset);
			glfwCustomTitlebarSetGroupSpacing(mp_window, i, m_props.custom_titlebar_props.button_groups[i].inner_spacing);
			glfwCustomTitlebarAddButtons(mp_window, i, start);
		}
	}

	IMAF::Application::ScreenRect Application::GetMainApplicationScreen()
	{
		MONITORINFOEXW monitor_info = {0};
		monitor_info.cbSize = sizeof(MONITORINFOEXW);
		HMONITOR monitor = MonitorFromWindow(glfwGetWin32Window(mp_window), MONITOR_DEFAULTTONEAREST);
		GetMonitorInfoW(monitor, &monitor_info);
		return { monitor_info.rcMonitor.left, monitor_info.rcMonitor.top, monitor_info.rcMonitor.right, monitor_info.rcMonitor.bottom };
	}

	void Application::RemovePanel(uint64_t id)
	{
		m_panels_mutex.lock();
		std::erase_if(m_panels, [id](std::shared_ptr<IMAF::Panel> ptr) -> bool { return ptr->GetId() == id ? true : false; });
		m_panels_mutex.unlock();
	}

	GLFWwindow* Application::GetWindowHandle() const
	{
		return mp_window;
	}

	void Application::AddDefDockingSetup(std::function<void(ImGuiID,bool&)> setup_func)
	{
		mp_def_docking = setup_func;
	}

	void Application::__CallScaleCallback(float x, float y)
	{
		if (x != m_dpi_scale)
		{
			if (mp_scaler)
			{
				mp_scaler->SetMainWindowScale((x < y ? x : y));
				mp_scaler->UpdateWindowScales();
			}

			float style_scale = x / m_dpi_scale;
			m_dpi_scale = x;

			ImGui::GetStyle().ScaleAllSizes(style_scale);

			if (m_props.custom_titlebar)
			{
				if (m_props.custom_titlebar_props.titlebar_scaling_f)
					m_props.custom_titlebar_props.titlebar_scaling_f(&m_props.custom_titlebar_props, style_scale);

				UpdateGLFWTitlebarRects();
			}
		}
	}

	ImGuiID Application::GetDockspaceId() const
	{
		return m_dockspace_id == 0 ? 0 : m_dockspace_id;
	}

	void DefCustomTitlebarDrawMAC(const AppProperties* app_props, GLFWwindow* window)
	{
		int w, h;
		glfwGetWindowSize(window, &w, &h);
		int x, y;
		glfwGetWindowPos(window, &x, &y);

		const Titlebar_Properties* props = &app_props->custom_titlebar_props;

		ImGui::SetNextWindowSize({ (float)w,(float)props->height });
		ImGui::SetNextWindowPos({ (float)x,(float)y });
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0,0 });
		ImGui::PushStyleColor(ImGuiCol_WindowBg, RGB2_IMVEC4(37, 37, 37));
		IMAF::Begin("##CustomTitlebar", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoDocking);
		const GLFWcustomtitlebar* p = glfwGetCustomTitlebarProperties(window);

		ImGui::PushStyleColor(ImGuiCol_Button, RGBA2_IMVEC4(37, 37, 37, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 35);

		for (size_t i = 0; i < props->button_groups.size(); i++)
		{
			if (props->button_groups[i].buttons.size() == 0)
				continue;

			float start_pos = 0;

			switch (props->button_groups[i].align)
			{
			case IMAF::GroupAlign::Left:
				start_pos = (float)w * props->button_groups[i].edge_offset;
				break;
			case IMAF::GroupAlign::Right:
				start_pos = w - props->button_groups[i].edge_offset * w - props->button_groups[i].buttons[0].width;
				break;
			case IMAF::GroupAlign::Center:
			{
				int width_sum = 0;
				
				for (const auto& i : props->button_groups[i].buttons)
				{
					width_sum += i.width;
				}
				width_sum += (props->button_groups[i].buttons.size() - 1) * props->button_groups[i].inner_spacing;
				start_pos = (float)(w - width_sum) / 2.f;
				break;
			}
			}

			for (const auto& btn : props->button_groups[i].buttons)
			{
				ImVec2 button_pos;
				ImVec2 button_size;
				button_pos.x = start_pos;
				button_pos.y = btn.top_offset;
				button_size.x = btn.width;
				button_size.y = btn.height;

				switch (btn.type)
				{
				case IMAF::ButtonType::Close:
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, RGBA2_IMVEC4(195, 65, 65, 255));
					ImGui::PushStyleColor(ImGuiCol_Button, RGBA2_IMVEC4(175, 55, 55, 255));
					ImGui::SetCursorPos(button_pos);
					ImGui::Button("##X", button_size);
					ImGui::PopStyleColor(2);
					break;
				case IMAF::ButtonType::Minimize:
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, RGBA2_IMVEC4(129, 227, 91, 255));
					ImGui::PushStyleColor(ImGuiCol_Button, RGBA2_IMVEC4(106, 226, 59, 255));
					ImGui::SetCursorPos(button_pos);
					ImGui::Button("##_", button_size);
					ImGui::PopStyleColor(2);
					break;
				case IMAF::ButtonType::Maximize:
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, RGBA2_IMVEC4(255, 196, 107, 255));
					ImGui::PushStyleColor(ImGuiCol_Button, RGBA2_IMVEC4(255, 175, 52, 255));
					ImGui::SetCursorPos(button_pos);
					ImGui::Button("##O", button_size);
					ImGui::PopStyleColor(2);
					break;
				default:
					break;
				}

				if (props->button_groups[i].align == IMAF::GroupAlign::Center || props->button_groups[i].align == IMAF::GroupAlign::Left)
					start_pos += btn.width + props->button_groups[i].inner_spacing;
				else if (props->button_groups[i].align == IMAF::GroupAlign::Right)
					start_pos -= btn.width + props->button_groups[i].inner_spacing;
			}
		}

		ImVec2 text_pos;
		text_pos.x = (float)w / 2 - ImGui::CalcTextSize(app_props->name).x / 2;
		text_pos.y = props->height / 2.f - ImGui::CalcTextSize(app_props->name).y / 2;

		ImGui::SetCursorPos(text_pos);
		ImGui::Text(app_props->name);

		ImDrawList* DrawList = ImGui::GetWindowDrawList();
		DrawList->AddLine({ (float)x,(float)(props->height - 2 + y) }, { (float)(w+x),(float)(props->height - 2 + y) }, IM_COL32(73,73,73,255), 3.0f);

		ImGui::PopStyleVar();
		ImGui::PopStyleColor();

		IMAF::End();
		ImGui::PopStyleVar();
		ImGui::PopStyleColor();
	}

	void DefCustomTitlebarDraw(const AppProperties* app_props, GLFWwindow* window)
	{
		int w, h;
		glfwGetWindowSize(window, &w, &h);
		int x, y;
		glfwGetWindowPos(window, &x, &y);

		const Titlebar_Properties* props = &app_props->custom_titlebar_props;

		ImGui::SetNextWindowSize({ (float)w,(float)props->height });
		ImGui::SetNextWindowPos({ (float)x,(float)y });
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0,0 });
		ImGui::PushStyleColor(ImGuiCol_WindowBg, RGB2_IMVEC4(37, 37, 37));
		IMAF::Begin("##CustomTitlebar", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoDocking);
		const GLFWcustomtitlebar* p = glfwGetCustomTitlebarProperties(window);

		ImGui::PushStyleColor(ImGuiCol_Button, RGBA2_IMVEC4(37, 37, 37, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 35);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 0,0 });

		for (size_t i = 0; i < props->button_groups.size(); i++)
		{
			if (props->button_groups[i].buttons.size() == 0)
				continue;

			float start_pos = 0;

			switch (props->button_groups[i].align)
			{
			case IMAF::GroupAlign::Left:
				start_pos = (float)w * props->button_groups[i].edge_offset;
				break;
			case IMAF::GroupAlign::Right:
				start_pos = w - props->button_groups[i].edge_offset * w - props->button_groups[i].buttons[0].width;
				break;
			case IMAF::GroupAlign::Center:
			{
				int width_sum = 0;

				for (const auto& i : props->button_groups[i].buttons)
				{
					width_sum += i.width;
				}
				width_sum += (props->button_groups[i].buttons.size() - 1) * props->button_groups[i].inner_spacing;
				start_pos = (float)(w - width_sum) / 2.f;
				break;
			}
			}

			for (const auto& btn : props->button_groups[i].buttons)
			{
				ImVec2 button_pos;
				ImVec2 button_size;
				button_pos.x = start_pos;
				button_pos.y = btn.top_offset;
				button_size.x = btn.width;
				button_size.y = btn.height;

				switch (btn.type)
				{
				case IMAF::ButtonType::Close:
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, RGBA2_IMVEC4(195, 65, 65, 255));
					ImGui::SetCursorPos(button_pos);
					ImGui::Button("X", button_size);
					ImGui::PopStyleColor();
					break;
				case IMAF::ButtonType::Minimize:
					ImGui::SetCursorPos(button_pos);
					ImGui::Button("_", button_size);
					break;
				case IMAF::ButtonType::Maximize:
					ImGui::SetCursorPos(button_pos);
					ImGui::Button("O", button_size);
					break;
				default:
					break;
				}

				if (props->button_groups[i].align == IMAF::GroupAlign::Center || props->button_groups[i].align == IMAF::GroupAlign::Left)
					start_pos += btn.width + props->button_groups[i].inner_spacing;
				else if (props->button_groups[i].align == IMAF::GroupAlign::Right)
					start_pos -= btn.width + props->button_groups[i].inner_spacing;
			}
		}

		ImVec2 text_pos;
		text_pos.x = (float)w / 2 - ImGui::CalcTextSize(app_props->name).x / 2;
		text_pos.y = props->height / 2.f - ImGui::CalcTextSize(app_props->name).y / 2;

		ImGui::SetCursorPos(text_pos);
		ImGui::Text(app_props->name);

		ImDrawList* DrawList = ImGui::GetWindowDrawList();
		DrawList->AddLine({ (float)x,(float)(props->height - 2 + y) }, { (float)(w + x),(float)(props->height - 2 + y) }, IM_COL32(73, 73, 73, 255), 3.0f);

#ifdef DRAW_CT_EXCLUSIONS
		for (const auto& i : props->exclusions)
		{
			ImVec2 start_pos = { (float)(i.start_offset * w + x),(float)(i.top_offset + y) };
			ImVec2 end_pos = { (float)(i.start_offset * w + x + i.width),(float)(i.top_offset + i.height + y) };
			DrawList->AddRectFilled(start_pos, end_pos, IM_COL32(0, 175, 0, 100));
		}
#endif // DRAW_CT_EXCLUSIONS

		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor();

		IMAF::End();
		ImGui::PopStyleVar();
		ImGui::PopStyleColor();
	}

	void DefCustomTitlebarScaling(Titlebar_Properties* out_props, float scale)
	{
		//Scale button width and titlebar height
		out_props->height *= scale;

		for (auto& i : out_props->exclusions)
		{
			i.height *= scale;
			i.width *= scale;
			i.top_offset *= scale;
		}

		for (auto& i : out_props->button_groups)
		{
			for (auto& i : i.buttons)
			{
				i.height *= scale;
				i.width *= scale;
				i.top_offset *= scale;
			}
		}
	}

	bool Begin(const char* name, bool* p_open, ImGuiWindowFlags flags)
	{
		if (g_app->mp_scaler == nullptr)
			return ImGui::Begin(name, p_open, flags);

		g_app->mp_scaler->SetCurrentID(name);

		ImGui::PushFont(g_app->m_fonts.at(g_app->mp_scaler->GetWindowScale(name))[FONT_NORMAL]);

		g_app->m_detached_window_scale = g_app->mp_scaler->GetWindowScale(name) / g_app->m_dpi_scale;
		
		if (g_app->m_detached_window_scale != 1.f)
			ImGui::GetStyle().ScaleAllSizes(g_app->m_detached_window_scale);

		bool ret = ImGui::Begin(name, p_open, flags);
		
		if (g_app->mp_scaler->IsFreshWindow(name))
		{
			ImVec2 current_size = ImGui::GetWindowSize();
			ImVec2 new_size = { current_size.x - 1, current_size.y - 1 };
			ImGui::SetWindowSize(new_size);
		}

		return ret;
	}

	ImFont* GetFont(int type)
	{
		if (g_app->mp_scaler == nullptr)
			return g_app->m_fonts.begin()->second[type];

		return g_app->m_fonts.at(g_app->mp_scaler->GetCurrentScale())[type];
	}

	void End()
	{
		if (g_app->mp_scaler == nullptr)
		{
			ImGui::End();
			return;
		}

		ImGui::PopFont();

		g_app->mp_scaler->SetCurrentID(std::string());

		ImGui::End();

		if (g_app->m_detached_window_scale != 1.f)
		{
			ImGui::GetStyle().ScaleAllSizes(1.f / g_app->m_detached_window_scale);
			g_app->m_detached_window_scale = 1.f;
		}
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
		colors[ImGuiCol_ResizeGrip] = RGBA2_IMVEC4(186, 124, 238, 180);
		colors[ImGuiCol_ResizeGripActive] = RGBA2_IMVEC4(199, 129, 255, 200);
		colors[ImGuiCol_ResizeGripHovered] = RGB2_IMVEC4(106, 49, 132);
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
		colors[ImGuiCol_ResizeGrip] = ImVec4(0.82f, 0.82f, 0.82f, 90.00f);
		colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.71f, 0.71f, 0.71f, 170.00f);
		colors[ImGuiCol_ResizeGripActive] = ImVec4(0.30f, 0.30f, 0.30f, 240.00f);
	}
}