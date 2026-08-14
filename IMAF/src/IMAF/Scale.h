#pragma once

#include <unordered_map>
#include <string>
#include <atomic>
#include <imgui.h>
#include <utility>

namespace IMAF 
{
	namespace Scale
	{
		int Val(int x);
		ImVec2 Val(int x,int y);
		float Val(float x);
		ImVec2 Val(float x,float y);

		void InitScaler();
		void DestroyScaler();

		class Scaler
		{
		public:
			float Val(float x) const;
			int Val(int x) const;

			void SetMainWindowScale(float scale);

			void SetCurrentID(std::string id);
			std::string GetCurrentID() const;

			bool IsFreshWindow(std::string id) const;

			float GetWindowScale(std::string id) const;
			float GetCurrentScale() const;

			void UpdateWindowScales();

		private:
			friend void InitScaler();
			Scaler();
			friend void DestroyScaler();
			~Scaler();

			void Setup();
			float GetMonitorScale(int monitor);
			friend void NewWindowCallback(ImGuiViewport* viewport);

		private:
			std::atomic<float> m_main_window_scale = 1.0f;
			std::unordered_map<std::string, std::pair<float,bool>> m_windows;
			std::unordered_map<int, float> m_monitors;
			std::string m_curr_id;
		};

		Scaler* GetScaler();
	}

	using Scale::Val;
}