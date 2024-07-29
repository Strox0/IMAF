#pragma once

#include "imgui_internal.h"
#include <unordered_map>
#include <string>
#include <atomic>

namespace IMAF 
{
	namespace Scale
	{
		int Val(int x);
		ImVec2 Val(int x,int y);
		float Val(float x);
		ImVec2 Val(float x,float y);

		void InitScaler();

		class Scaler
		{
		public:
			Scaler();
			~Scaler();

			float Val(float x) const;
			int Val(int x) const;

			void SetMainWindowScale(float scale);

			void Shutdown() { shutdown = true; }

			void SetCurrentID(std::string id);
			std::string GetCurrentID() const;

			bool IsFreshWindow(std::string id) const;

			float GetWindowScale(std::string id) const;
			float GetCurrentScale() const;

			void UpdateWindowScales();

		private:

			void Setup();
			friend void NewWindowCallback(ImGuiViewport* viewport);

		private:
			std::atomic<float> m_main_window_scale = 1.0f;
			std::unordered_map<std::string, std::pair<float,bool>> m_windows;
			std::unordered_map<short, float> m_monitors;
			bool shutdown = false;
			std::string m_curr_id;
		};

		Scaler* GetScaler();
	}

	using Scale::Val;
}