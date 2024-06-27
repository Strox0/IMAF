#include "IMAF/Scale.h"
#include <cmath>
#include <thread>
#include <Windows.h>

#include "GLFW/glfw3.h"

IMAF::Scale::Scaler* g_scaler = nullptr;

//BOOL monitorenumproc(HMONITOR unnamedParam1, HDC unnamedParam2, LPRECT unnamedParam3, LPARAM unnamedParam4);

int IMAF::Scale::Val(int x)
{
	if (g_scaler == nullptr)
		return x;
	return g_scaler->Val(x);
}

ImVec2 IMAF::Scale::Val(int x, int y)
{
	if (g_scaler == nullptr)
		return ImVec2(x, y);

	return ImVec2(g_scaler->Val(x), g_scaler->Val(y));
}

float IMAF::Scale::Val(float x)
{
	if (g_scaler == nullptr)
		return x;
	return g_scaler->Val(x);
}

ImVec2 IMAF::Scale::Val(float x, float y)
{
	if (g_scaler == nullptr)
		return ImVec2(x, y);

	return ImVec2(g_scaler->Val(x), g_scaler->Val(y));
}

void IMAF::Scale::InitScaler()
{
	if (g_scaler == nullptr)
	{
		g_scaler = new Scaler();
	}
}

void IMAF::Scale::NewWindowCallback(ImGuiViewport* viewport)
{
	assert(viewport != nullptr);
	ImGuiContext* ctx = ImGui::GetCurrentContext();
	
	ImGuiWindow* window = nullptr;

	for (int i = 0; i < ctx->Windows.Size; i++)
	{
		if (ctx->Windows[i]->Viewport == nullptr)
			continue;
		if (ctx->Windows[i]->Viewport->ID == viewport->ID)
		{
			window = ctx->Windows[i];
			break;
		}
	}
	
	if (window == nullptr)
	{
		MessageBoxA(NULL, "Window associated with viewport not found", "Critical Error", MB_OK | MB_ICONERROR);
		ExitProcess(1);
	}

	if (!window->ViewportOwned)
		g_scaler->m_windows[std::string(window->Name)] = g_scaler->m_main_window_scale;
	else
		g_scaler->m_windows[std::string(window->Name)] = g_scaler->m_monitors[window->Viewport->PlatformMonitor];
}

IMAF::Scale::Scaler* IMAF::Scale::GetScaler()
{
	return g_scaler;
}

IMAF::Scale::Scaler::Scaler() : m_main_window_scale(1.f)
{
	std::thread t(&Scaler::Setup, this);
	t.detach();
}

IMAF::Scale::Scaler::~Scaler()
{
	g_scaler = nullptr;
}

float IMAF::Scale::Scaler::Val(float x) const
{
	if (m_curr_id.empty() || m_windows.find(m_curr_id) == m_windows.end())
		return x * m_main_window_scale;

	return x * m_windows.at(m_curr_id);
}

int IMAF::Scale::Scaler::Val(int x) const
{
	if (m_curr_id.empty() || m_windows.find(m_curr_id) == m_windows.end())
		return std::lround((float)x * m_main_window_scale);

	return std::lround((float)x * m_windows.at(m_curr_id));
}

void IMAF::Scale::Scaler::SetMainWindowScale(float scale)
{
	m_main_window_scale = scale;
}

void IMAF::Scale::Scaler::SetCurrentID(std::string id)
{
	m_curr_id = id;
}

float IMAF::Scale::Scaler::GetWindowScale(std::string id) const
{
	if (id.empty() || m_windows.find(id) == m_windows.end())
		return m_main_window_scale;

	return m_windows.at(id);
}

float IMAF::Scale::Scaler::GetCurrentScale() const
{
	if (m_curr_id.empty() || m_windows.find(m_curr_id) == m_windows.end())
		return m_main_window_scale;

	return m_windows.at(m_curr_id);
}

void IMAF::Scale::Scaler::Setup()
{
	while (ImGui::GetCurrentContext() == nullptr)
	{
		Sleep(10);
	}

	ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();

	for (int i = 0; i < platform_io.Monitors.size(); ++i)
	{
		float xs, ys;
		glfwGetMonitorContentScale((GLFWmonitor*)platform_io.Monitors[i].PlatformHandle, &xs, &ys);

		m_monitors[i] = (xs < ys ? xs : ys);
	}

	platform_io.Platform_OnChangedViewport = IMAF::Scale::NewWindowCallback;

	//std::vector<HMONITOR> monitors;
	//EnumDisplayMonitors(NULL, NULL, monitorenumproc, (long long)&monitors);
	//for (size_t i = 0; i < monitors.size(); i++) 
	//{
	//	MONITORINFO monitorInfo;
	//	monitorInfo.cbSize = sizeof(MONITORINFO);

	//	if (GetMonitorInfo(monitors[i], &monitorInfo)) 
	//	{
	//		for (int j = 0; j < platform_io.Monitors.size(); ++j) {
	//			const ImGuiPlatformMonitor& imguiMonitor = platform_io.Monitors[j];

	//			if (monitorInfo.rcMonitor.left == imguiMonitor.MainPos.x && monitorInfo.rcMonitor.top == imguiMonitor.MainPos.y) 
	//			{
	//				UINT dVal, dpiY;
	//				HRESULT hr = GetDpiForMonitor(monitors[i], MDT_EFFECTIVE_DPI, &dVal, &dpiY);
	//				if (SUCCEEDED(hr))
	//				{
	//					m_monitors[j] = dVal / 96.0f;
	//				}
	//			}
	//		}
	//	}
	//}
}

void IMAF::Scale::Scaler::UpdateWindowScales()
{
	ImGuiContext* ctx = ImGui::GetCurrentContext();

	for (const auto& window : ctx->Windows)
	{
		if (!window->ViewportOwned)
			m_windows[std::string(window->Name)] = m_main_window_scale;
		else
			m_windows[std::string(window->Name)] = m_monitors[window->Viewport->PlatformMonitor];
	}
}

//BOOL monitorenumproc(HMONITOR unnamedParam1, HDC unnamedParam2, LPRECT unnamedParam3, LPARAM unnamedParam4)
//{
//	std::vector<HMONITOR>* monitors = (std::vector<HMONITOR>*)unnamedParam4;
//	monitors->push_back(unnamedParam1);
//	return TRUE;
//}