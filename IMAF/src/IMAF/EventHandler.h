#pragma once
#include "Events.h"
#include "IMAF/Random.h"
#include <vector>
#include <unordered_map>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <unordered_set>
#include <atomic>
#include <thread>

namespace IMAF
{
	class Application;
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


		private:
			void Run();
			void End();

		private:
			struct EventPair
			{
				EventCallback callback;
				SubID sub_id;
				EventPair(EventCallback cb, SubID id) : callback(cb), sub_id(id) {};
			};

			EventHandler();
			std::queue<Event> m_event_pool;
			std::unordered_map<EventID, std::vector<EventPair>> m_events_map;
			std::unordered_set<SubID> m_ids;
			std::unordered_map<SubID, size_t> m_position_map;
			IMAF::Utils::Random::Number<IMAF::Events::SubID> m_random;

			std::condition_variable m_cv;
			std::mutex m_mutex_queue;
			std::mutex m_mutex_events;
			std::mutex m_mutex_ids;
			std::atomic_bool m_end = false;
			std::thread m_t;
		};
	}
}