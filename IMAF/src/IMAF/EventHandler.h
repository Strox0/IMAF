#pragma once
#include "Events.h"
#include <vector>
#include <unordered_map>
#include <queue>
#include <mutex>
#include <condition_variable>

namespace IMAF
{
	namespace Events
	{
		class EventHandler
		{
		public:
			EventHandler(const EventHandler&) = delete;
			EventHandler& operator=(const EventHandler&) = delete;
			EventHandler(EventHandler&&) = delete;
			EventHandler& operator=(EventHandler&&) = delete;
			static EventHandler& Get()
			{
				static EventHandler instance;
				return instance;
			}
			~EventHandler();

			void Dispatch(const Event& event);

			EventSubID Subscribe(EventID event_id, EventCallback callback);

			void Unsubscribe(EventSubID sub_id);

			void RegisterEvent(EventID event_id);

			void End();

		private:
			void Run();

		private:
			EventHandler();
			std::queue<Event> m_event_pool;
			std::unordered_map<EventID, std::vector<EventCallback>> m_events_map;

			std::condition_variable m_cv;
			std::mutex m_mutex_queue;
			std::mutex m_mutex_events;
			std::thread m_t;
			bool m_end = false;
		};
	}
}