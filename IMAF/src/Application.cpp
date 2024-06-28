#define IMGUI_DEFINE_MATH_OPERATORS
#include "IMAF/Application.h"

#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "imgui_internal.h"

#include "GLES3/gl3.h"
#include "GLFW/glfw3.h"

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

	app->ReCaclWindowSize();
}

namespace IMAF
{
	Application::Application(const AppProperties& props) : m_props(props) 
	{
		//m_props.custom_titlebar = false;//DISABLE CUSTOM TITLEBAR
		if (Init())
		{
			MessageBoxA(NULL, "Application Initalization Failed", "Fatal Error", MB_OK | MB_ICONERROR);
			Shutdown();
			ExitProcess(1);
		}

		g_app = this;

		if (m_props.dpi_aware)
		{
			Scale::InitScaler();
			mp_scaler = Scale::GetScaler();
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

		if (m_props.maximized)
		{
			int screen_width = GetSystemMetrics(SM_CXSCREEN);
			int screen_height = GetSystemMetrics(SM_CYSCREEN);
			m_props.width = screen_width;
			m_props.height = screen_height;
		}

		if (m_props.width <= 0 || m_props.height <= 0)
		{
			int screen_width = GetSystemMetrics(SM_CXSCREEN);
			int screen_height = GetSystemMetrics(SM_CYSCREEN);
			m_props.width = screen_width * 0.65;
			m_props.height = screen_height * 0.75;
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

		GLFWmonitor* monitor = nullptr;
		if (m_props.fullscreen)
			monitor = glfwGetPrimaryMonitor();

		mp_window = glfwCreateWindow(m_props.width, m_props.height, m_props.name, monitor, NULL); //Create Window
		if (mp_window == nullptr)
			return true;

		m_screen_size = GetMainApplicationScreenSize();

		glfwSetWindowUserPointer(mp_window, this);
		glfwMakeContextCurrent(mp_window);
		glfwSwapInterval(1);

		if (m_props.custom_titlebar)
		{
			UpdateGLFWTitlebarRects();

			glfwCustomTitlebarAddExclusion(mp_window, (GLFWChainRect*)m_props.custom_titlebar_props.exclusions);

			glfwSetCustomTitleBar(mp_window, true);

			if (m_props.custom_titlebar_props.titlebar_draw_f == nullptr)
				m_props.custom_titlebar_props.titlebar_draw_f = DefCustomTitlebarDraw;

			if (m_props.custom_titlebar_props.titlebar_scaling_f == nullptr)
				m_props.custom_titlebar_props.titlebar_scaling_f = DefCustomTitlebarScaling;
		}

		if (m_props.dpi_aware)
		{
			glfwSetWindowContentScaleCallback(mp_window, __DPICallBack);
		}

		if (m_props.min_height != 0 && m_props.min_width != 0 && m_props.resizeable)
		{
			glfwSetWindowSizeLimits(mp_window, m_props.min_width, m_props.min_height, GLFW_DONT_CARE, GLFW_DONT_CARE);
		}

		if (m_props.maximized)
			glfwSetWindowPos(mp_window, 0, 0);

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

		for (const auto& i : platform_io.Monitors)
		{
			std::vector<ImFont*> fonts;
			float size = std::floorf((float)m_props.font_size * i.DpiScale);
			fonts.push_back(io.Fonts->AddFontFromMemoryTTF((void*)&g_FontRegular[0],	sizeof(g_FontRegular),	size, &fontConfig));
			fonts.push_back(io.Fonts->AddFontFromMemoryTTF((void*)&g_FontLight[0],		sizeof(g_FontLight),	size, &fontConfig));
			fonts.push_back(io.Fonts->AddFontFromMemoryTTF((void*)&g_FontMedium[0],		sizeof(g_FontMedium),	size, &fontConfig));
			fonts.push_back(io.Fonts->AddFontFromMemoryTTF((void*)&g_FontSemibold[0],	sizeof(g_FontSemibold), size, &fontConfig));
			fonts.push_back(io.Fonts->AddFontFromMemoryTTF((void*)&g_FontBold[0],		sizeof(g_FontBold),		size, &fontConfig));
			fonts.push_back(io.Fonts->AddFontFromMemoryTTF((void*)&g_FontExtrabold[0],	sizeof(g_FontExtrabold),size, &fontConfig));

			m_fonts[i.DpiScale] = fonts;
		}

		float xscale, yscale;
		glfwGetWindowContentScale(mp_window, &xscale, &yscale);
		io.FontDefault = m_fonts[xscale][0];
		style.ScaleAllSizes(xscale);
		m_dpi_scale = xscale;

		int w, h;
		glfwGetWindowSize(mp_window, &w, &h);
		glfwSetWindowSize(mp_window, w * xscale, h * yscale);

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

	void Application::SetUp(std::function<void()> func)
	{
		mp_setup_func = func;
	}

	void Application::Run()
	{
		if (mp_setup_func)
			mp_setup_func();

		if (mp_scaler)
		{
			float xscale, yscale;
			glfwGetWindowContentScale(mp_window, &xscale, &yscale);
			mp_scaler->SetMainWindowScale((xscale < yscale ? xscale : yscale));
		}

		m_exited = false;
		while (!glfwWindowShouldClose(mp_window) && !m_should_exit)
		{
			BeginRender();

			if (m_props.custom_titlebar)
				m_props.custom_titlebar_props.titlebar_draw_f(&m_props,mp_window);

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
		m_exited = true;
	}

	void Application::Shutdown()
	{
		m_should_exit = true;
		while (!m_exited)
		{
			Sleep(5);
		}

		if (mp_scaler)
			mp_scaler->Shutdown();

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
		props.custom_titlebar = false;
		props.font_size = 18;
		props.fullscreen = false;
		props.height = 500;
		props.width = 720;
		props.resizeable = true;
		props.name = "My Application";
		props.imgui_docking = false;
		props.gen_ini = false;
		props.center_window = true;
		props.maximized = false;
		props.dpi_aware = true;	
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
		GLFWRect close_r{ 0 }, min_r{ 0 }, max_r{ 0 };

		close_r.top = m_props.custom_titlebar_props.close_button.y;
		close_r.left = m_props.custom_titlebar_props.close_button.x;
		close_r.right = close_r.left + m_props.custom_titlebar_props.close_button.width;
		close_r.bottom = m_props.custom_titlebar_props.close_button.height == -1 ? m_props.custom_titlebar_props.height : m_props.custom_titlebar_props.close_button.height;

		min_r.top = m_props.custom_titlebar_props.minimize_button.y;
		min_r.left = m_props.custom_titlebar_props.minimize_button.x;
		min_r.right = min_r.left + m_props.custom_titlebar_props.minimize_button.width;
		min_r.bottom = m_props.custom_titlebar_props.minimize_button.height == -1 ? m_props.custom_titlebar_props.height : m_props.custom_titlebar_props.minimize_button.height;

		max_r.top = m_props.custom_titlebar_props.maximize_button.y;
		max_r.left = m_props.custom_titlebar_props.maximize_button.x;
		max_r.right = max_r.left + m_props.custom_titlebar_props.maximize_button.width;
		max_r.bottom = m_props.custom_titlebar_props.maximize_button.height == -1 ? m_props.custom_titlebar_props.height : m_props.custom_titlebar_props.maximize_button.height;

		glfwSetCustomTitlebarHeight(mp_window, m_props.custom_titlebar_props.height);
		glfwSetCustomTitlebarButton(mp_window, GLFW_CT_CLOSE_BUTTON, &close_r);
		glfwSetCustomTitlebarButton(mp_window, GLFW_CT_MINIMIZE_BUTTON, &min_r);
		glfwSetCustomTitlebarButton(mp_window, GLFW_CT_MAXIMIZE_BUTTON, &max_r);
	}

	IMAF::Application::SizeRect Application::GetMainApplicationScreenSize()
	{
		MONITORINFOEXW monitor_info;
		monitor_info.cbSize = sizeof(MONITORINFOEXW);
		HMONITOR monitor = MonitorFromWindow(glfwGetWin32Window(mp_window), MONITOR_DEFAULTTONEAREST);
		GetMonitorInfoW(monitor, &monitor_info);
		return { monitor_info.rcMonitor.right - monitor_info.rcMonitor.left, monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top };
	}

	void Application::RemovePanel(uint64_t id)
	{
		m_panels_mutex.lock();
		std::erase_if(m_panels, [id](std::shared_ptr<IMAF::Panel> ptr) -> bool { return ptr->GetId() == id ? true : false; });
		m_panels_mutex.unlock();
	}

    void Application::AddDefDockingSetup(std::function<void(ImGuiID,bool&)> setup_func)
    {
		mp_def_docking = setup_func;
    }

	void Application::__CallScaleCallback(float x, float y)
	{
		mp_scaler->SetMainWindowScale((x < y ? x : y));
	}

	void Application::ReCaclWindowSize()
	{
		SizeRect new_screen_size = GetMainApplicationScreenSize();

		if (new_screen_size != m_screen_size)
		{			
			int w, h;
			glfwGetWindowSize(mp_window, &w, &h);

			float new_dpi_scale, y;
			glfwGetWindowContentScale(mp_window, &new_dpi_scale, &y);

			float style_scale = new_dpi_scale / m_dpi_scale;
			m_dpi_scale = new_dpi_scale;

			float x_ratio = (float)w / (float)m_screen_size.width;
			float y_ratio = (float)h / (float)m_screen_size.height;

			ImGui::GetStyle().ScaleAllSizes(style_scale);

			if (m_props.custom_titlebar)
			{
				if (m_props.custom_titlebar_props.titlebar_scaling_f)
					m_props.custom_titlebar_props.titlebar_scaling_f(&m_props.custom_titlebar_props, style_scale, mp_window);

				UpdateGLFWTitlebarRects();
			}

			glfwSetWindowSize(mp_window, std::lround((float)new_screen_size.width * x_ratio), std::lround((float)new_screen_size.height * y_ratio));

			m_screen_size = new_screen_size;
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

		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, RGBA2_IMVEC4(195, 65, 65, 255));
		ImGui::PushStyleColor(ImGuiCol_Button, RGBA2_IMVEC4(175, 55, 55, 255));
		ImGui::SetCursorPos({ (float)p->closeButton.left,(float)p->closeButton.top });
		ImGui::Button("##X", { (float)props->close_button.width,(float)props->close_button.height });
		ImGui::PopStyleColor(2);

		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, RGBA2_IMVEC4(129, 227, 91, 255));
		ImGui::PushStyleColor(ImGuiCol_Button, RGBA2_IMVEC4(106, 226, 59, 255));
		ImGui::SetCursorPos({ (float)p->minimizeButton.left,(float)p->minimizeButton.top });
		ImGui::Button("##_", { (float)props->minimize_button.width,(float)props->minimize_button.height });
		ImGui::PopStyleColor(2);

		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, RGBA2_IMVEC4(255, 196, 107, 255));
		ImGui::PushStyleColor(ImGuiCol_Button, RGBA2_IMVEC4(255, 175, 52, 255));
		ImGui::SetCursorPos({ (float)p->maximizeButton.left,(float)p->maximizeButton.top });
		ImGui::Button("##O", { (float)props->maximize_button.width,(float)props->maximize_button.height });
		ImGui::PopStyleColor(2);

		ImVec2 text_pos;
		text_pos.x = w / 2 - ImGui::CalcTextSize(app_props->name).x / 2;
		text_pos.y = props->height / 2 - ImGui::CalcTextSize(app_props->name).y / 2;

		ImGui::SetCursorPos(text_pos);
		ImGui::Text(app_props->name);

		ImDrawList* DrawList = ImGui::GetWindowDrawList();
		DrawList->AddLine({ (float)x,(float)(props->height - 1 + y) }, { (float)(w+x),(float)(props->height - 1 + y) }, IM_COL32(73,73,73,255), 3.0f);

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

		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, RGBA2_IMVEC4(195, 65, 65, 255));
		ImGui::SetCursorPos({ (float)p->closeButton.left,(float)p->closeButton.top });
		ImGui::Button("X", { (float)props->close_button.width,(float)props->close_button.height });
		ImGui::PopStyleColor();

		ImGui::SetCursorPos({ (float)p->minimizeButton.left,(float)p->minimizeButton.top });
		ImGui::Button("_", { (float)props->minimize_button.width,(float)props->minimize_button.height });

		ImGui::SetCursorPos({ (float)p->maximizeButton.left,(float)p->maximizeButton.top });
		ImGui::Button("O", { (float)props->maximize_button.width,(float)props->maximize_button.height });

		ImVec2 text_pos;
		text_pos.x = w / 2 - ImGui::CalcTextSize(app_props->name).x / 2;
		text_pos.y = props->height / 2 - ImGui::CalcTextSize(app_props->name).y / 2;

		ImGui::SetCursorPos(text_pos);
		ImGui::Text(app_props->name);

		ImDrawList* DrawList = ImGui::GetWindowDrawList();
		DrawList->AddLine({ (float)x,(float)(props->height - 1 + y) }, { (float)(w + x),(float)(props->height - 1 + y) }, IM_COL32(73, 73, 73, 255), 3.0f);

		// Draw Exclusion Rects:
		// 
		//GLFWChainRect* chain = p->exclusions;
		//while (chain != nullptr)
		//{
		//	DrawList->AddRectFilled({ (float)(chain->rect.left + x),(float)(chain->rect.top + y) }, { (float)(chain->rect.right + x),(float)(chain->rect.bottom + y) }, IM_COL32(0, 175, 0, 100));
		//	chain = chain->next;
		//}

		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor();

		IMAF::End();
		ImGui::PopStyleVar();
		ImGui::PopStyleColor();
	}

	void DefCustomTitlebarScaling(Titlebar_Properties* out_props, float scale, GLFWwindow* window)
	{
		//Get current button positions
		const GLFWcustomtitlebar* pt = glfwGetCustomTitlebarProperties(window);

		out_props->close_button.x = pt->closeButton.left;
		out_props->close_button.y = pt->closeButton.top;
		out_props->minimize_button.x = pt->minimizeButton.left;
		out_props->minimize_button.y = pt->minimizeButton.top;
		out_props->maximize_button.x = pt->maximizeButton.left;
		out_props->maximize_button.y = pt->maximizeButton.top;

		//Scale button width and titlebar height
		out_props->height *= scale;
		out_props->close_button.width *= scale;
		out_props->close_button.height = out_props->close_button.height == -1 ? -1 : out_props->close_button.height * scale;
		out_props->minimize_button.width *= scale;
		out_props->minimize_button.height = out_props->minimize_button.height == -1 ? -1 : out_props->minimize_button.height * scale;
		out_props->maximize_button.width *= scale;
		out_props->maximize_button.height = out_props->maximize_button.height == -1 ? -1 : out_props->maximize_button.height * scale;
	}

	bool Begin(const char* name, bool* p_open, ImGuiWindowFlags flags)
	{
		if (g_app->mp_scaler == nullptr)
			return ImGui::Begin(name, p_open, flags);

		g_app->mp_scaler->SetCurrentID(name);

		ImGui::PushFont(g_app->m_fonts.at(g_app->mp_scaler->GetWindowScale(name))[FONT_NORMAL]);

		return ImGui::Begin(name, p_open, flags);
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