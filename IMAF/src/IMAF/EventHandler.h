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
#include <memory>
#include <variant>

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
			void Notify(EventID id);

			EventSubID Subscribe(EventID event_id, EventCallback callback);
			EventSubID Subscribe(EventID event_id, NotifyCallback callback);

			void Unsubscribe(EventSubID sub_id);


		private:

			struct EventJob
			{
				EventCallback callback;
				NotifyCallback notify_callback;
				SubID sub_id;
				bool active;
				EventJob(EventCallback cb, SubID id) : callback(cb), sub_id(id), active(true), notify_callback(nullptr) {};
				EventJob(NotifyCallback cb, SubID id) : callback(nullptr), sub_id(id), active(true), notify_callback(cb) {};
			};

			struct UnsubSignal
			{
				std::condition_variable cv;
				std::mutex mutex;
				bool complete = false;
			};

			struct Unsub
			{
				EventSubID sub_id;
				std::shared_ptr<UnsubSignal> signal;
			};

			using Job = std::variant<Event, Unsub>;

			EventHandler();

			void Run();
			void End();
			void RemoveSub(const EventSubID& sub_id);
			void ProcessEvent(const Event& event);

			std::queue<Job> m_job_pool;
			std::unordered_map<EventID, std::vector<std::shared_ptr<EventJob>>> m_events_map;
			std::unordered_set<SubID> m_ids;
			IMAF::Utils::Random::Number<IMAF::Events::SubID> m_random;

			std::condition_variable m_cv;
			std::mutex m_mutex_queue;
			std::mutex m_mutex_events;
			std::mutex m_mutex_ids;
			std::atomic_bool m_end = false;
			std::thread::id m_worker_thread_id;
			bool m_worker_stopped = false;
			std::thread m_t;
		};
	}
}