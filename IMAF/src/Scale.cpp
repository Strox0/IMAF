#include "IMAF/Scale.h"
#include <cmath>
#include <Windows.h>

#include "GLFW/glfw3.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <cassert>
#include <string>
#include <unordered_set>
#include <unordered_map>

IMAF::Scale::Scaler* g_scaler = nullptr;

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

void IMAF::Scale::DestroyScaler()
{
	if (g_scaler != nullptr)
	{
		delete g_scaler;
	}
}

// ISSUE 22: Failures terminate the host process instead of returning structured errors.
void IMAF::Scale::NewWindowCallback(ImGuiViewport* viewport)
{
	assert(viewport != nullptr);
	ImGuiContext* ctx = ImGui::GetCurrentContext();
	static size_t callt = 0;
	callt++;
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

	if (g_scaler->m_windows[std::string(window->Name)].second)
		g_scaler->m_windows[std::string(window->Name)].second = false;

	if (!window->ViewportOwned)
	{
		bool fresh = g_scaler->m_windows[std::string(window->Name)].first != g_scaler->m_main_window_scale;
		g_scaler->m_windows[std::string(window->Name)].first = g_scaler->m_main_window_scale;
		g_scaler->m_windows[std::string(window->Name)].second = fresh;
	}
	else
	{
		float scale = g_scaler->GetMonitorScale(window->Viewport->PlatformMonitor);
		bool fresh = g_scaler->m_windows[std::string(window->Name)].first != scale;
		g_scaler->m_windows[std::string(window->Name)].first = scale;
		g_scaler->m_windows[std::string(window->Name)].second = fresh;
	}
}

IMAF::Scale::Scaler* IMAF::Scale::GetScaler()
{
	return g_scaler;
}

IMAF::Scale::Scaler::Scaler() : m_main_window_scale(1.f)
{
	Setup();
}

IMAF::Scale::Scaler::~Scaler()
{
	g_scaler = nullptr;
	ImGui::GetPlatformIO().Platform_OnChangedViewport = nullptr;
}

float IMAF::Scale::Scaler::Val(float x) const
{
	if (m_curr_id.empty() || m_windows.find(m_curr_id) == m_windows.end())
		return x * m_main_window_scale;

	return x * m_windows.at(m_curr_id).first;
}

int IMAF::Scale::Scaler::Val(int x) const
{
	if (m_curr_id.empty() || m_windows.find(m_curr_id) == m_windows.end())
		return std::lround((float)x * m_main_window_scale);

	return std::lround((float)x * m_windows.at(m_curr_id).first);
}

void IMAF::Scale::Scaler::SetMainWindowScale(float scale)
{
	m_main_window_scale = scale;
}

void IMAF::Scale::Scaler::SetCurrentID(std::string id)
{
	m_curr_id = id;
}

std::string IMAF::Scale::Scaler::GetCurrentID() const
{
	return m_curr_id;
}

bool IMAF::Scale::Scaler::IsFreshWindow(std::string id) const
{
	if (id.empty() || m_windows.find(id) == m_windows.end())
		return false;

	return m_windows.at(id).second;
}

float IMAF::Scale::Scaler::GetWindowScale(std::string id) const
{
	if (id.empty() || m_windows.find(id) == m_windows.end())
		return m_main_window_scale;

	return m_windows.at(id).first;
}

float IMAF::Scale::Scaler::GetCurrentScale() const
{
	if (m_curr_id.empty() || m_windows.find(m_curr_id) == m_windows.end())
		return m_main_window_scale;

	return m_windows.at(m_curr_id).first;
}

float IMAF::Scale::Scaler::GetMonitorScale(int monitor)
{
	ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
	if (monitor >= 0 && monitor < platform_io.Monitors.size() && platform_io.Monitors[monitor].PlatformHandle != nullptr)
	{
		float xs, ys;
		glfwGetMonitorContentScale((GLFWmonitor*)platform_io.Monitors[monitor].PlatformHandle, &xs, &ys);
		return m_monitors[monitor] = min(xs,ys);
	}

	auto it = m_monitors.find(monitor);
	return it != m_monitors.end() ? it->second : m_main_window_scale.load();
}

void IMAF::Scale::Scaler::Setup()
{
	ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();

	for (int i = 0; i < platform_io.Monitors.size(); ++i)
	{
		float xs, ys;
		glfwGetMonitorContentScale((GLFWmonitor*)platform_io.Monitors[i].PlatformHandle, &xs, &ys);

		m_monitors[i] = (xs < ys ? xs : ys);
	}

	platform_io.Platform_OnChangedViewport = IMAF::Scale::NewWindowCallback;
}

void IMAF::Scale::Scaler::UpdateWindowScales()
{
	ImGuiContext* ctx = ImGui::GetCurrentContext();
	std::unordered_set<std::string> active_windows;

	for (const auto& window : ctx->Windows)
	{
		if (!window->Active)
			continue;

		std::string window_name(window->Name);
		active_windows.insert(window_name);

		if (!window->ViewportOwned)
		{
			bool fresh = m_windows[window_name].first != m_main_window_scale;
			m_windows[window_name].first = m_main_window_scale;
			m_windows[window_name].second = fresh;
		}
		else
		{
			float scale = GetMonitorScale(window->Viewport->PlatformMonitor);
			bool fresh = m_windows[window_name].first != scale;
			m_windows[window_name].first = scale;
			m_windows[window_name].second = fresh;
		}
	}

	std::erase_if(m_windows, [&active_windows](const auto& window)
		{
			return !active_windows.contains(window.first);
		});
}