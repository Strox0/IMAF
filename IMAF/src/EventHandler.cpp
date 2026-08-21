#include "IMAF/EventHandler.h"
#include "IMAF/Events.h"
#include <limits>
#include <mutex>
#include <utility>
#include <vector>
#include <thread>
#include <variant>
#include <memory>

IMAF::Events::EventHandler::EventHandler() : m_t(&IMAF::Events::EventHandler::Run, this), m_random(1,std::numeric_limits<IMAF::Events::SubID>::max())
{
	
};

IMAF::Events::EventHandler::~EventHandler()
{
	End();
}

void IMAF::Events::EventHandler::Dispatch(const Event& event)
{
	{
		std::lock_guard<std::mutex> m(m_mutex_queue);
		m_job_pool.emplace(event);
	}

	m_cv.notify_one();
}

void IMAF::Events::EventHandler::Notify(EventID id)
{
	Event e;
	e.event_id = id;
	Dispatch(e);
}

IMAF::Events::EventSubID IMAF::Events::EventHandler::Subscribe(EventID event_id, EventCallback callback)
{
	SubID id = 0;

	{
		std::lock_guard<std::mutex> mids(m_mutex_ids);

		do
		{
			id = m_random.Gen();
		} while (m_ids.contains(id));
		m_ids.insert(id);
	}

	{
		std::lock_guard me(m_mutex_events);
		if (!m_events_map.contains(event_id))
		{
			m_events_map.emplace(event_id, std::vector<std::shared_ptr<EventJob>>());
		}

		m_events_map[event_id].emplace_back(std::make_shared<EventJob>(callback, id));
	}

	EventSubID subid;
	subid.event_id = event_id;
	subid.sub_id = id;
	
	return subid;
}

IMAF::Events::EventSubID IMAF::Events::EventHandler::Subscribe(EventID event_id, NotifyCallback callback)
{
	SubID id = 0;

	{
		std::lock_guard<std::mutex> mids(m_mutex_ids);

		do
		{
			id = m_random.Gen();
		} while (m_ids.contains(id));
		m_ids.insert(id);
	}

	{
		std::lock_guard me(m_mutex_events);
		if (!m_events_map.contains(event_id))
		{
			m_events_map.emplace(event_id, std::vector<std::shared_ptr<EventJob>>());
		}

		m_events_map[event_id].emplace_back(std::make_shared<EventJob>(callback, id));
	}

	EventSubID subid;
	subid.event_id = event_id;
	subid.sub_id = id;

	return subid;
}

void IMAF::Events::EventHandler::Unsubscribe(EventSubID sub_id)
{
	bool remove_directly = false;
	std::shared_ptr<UnsubSignal> signal;
	{
		std::lock_guard lock(m_mutex_queue);
		remove_directly = m_worker_stopped || m_worker_thread_id == std::this_thread::get_id();
		if (!remove_directly)
		{
			signal = std::make_shared<UnsubSignal>();
			m_job_pool.emplace(Unsub{ sub_id,signal });
		}
	}

	if (remove_directly)
	{
		RemoveSub(sub_id);
		return;
	}

	m_cv.notify_one();

	std::unique_lock lock(signal->mutex);
	signal->cv.wait(lock, [&signal]() { return signal->complete; });
}

void IMAF::Events::EventHandler::End()
{
	{
		std::lock_guard m(m_mutex_queue);
		m_end = true;
	}
	m_cv.notify_all();
	if (m_t.joinable() && m_t.get_id() != std::this_thread::get_id())
		m_t.join();
}

void IMAF::Events::EventHandler::RemoveSub(const EventSubID& sub_id)
{
	bool removed = false;
	{
		std::lock_guard me(m_mutex_events);
		auto map_it = m_events_map.find(sub_id.event_id);
		if (map_it == m_events_map.end())
			return;

		auto it = map_it->second.begin();
		while (it != map_it->second.end())
		{
			if ((*it)->sub_id == sub_id.sub_id)
			{
				(*it)->active = false;

				if (map_it->second.size() == 1)
					m_events_map.erase(map_it);
				else
					map_it->second.erase(it);

				removed = true;
				break;
			}
			++it;
		}
	}

	if (removed)
	{ 
		std::lock_guard mids(m_mutex_ids);
		m_ids.erase(sub_id.sub_id);
	}
}

void IMAF::Events::EventHandler::ProcessEvent(const Event& event)
{
	std::vector<std::shared_ptr<EventJob>> callbacks;
	{
		std::lock_guard me(m_mutex_events);
		auto it = m_events_map.find(event.event_id);
		if (it == m_events_map.end())
			return;
		callbacks = it->second;
	}

	for (const auto& job : callbacks)
	{
		if (!job->active)
			continue;

		try
		{
			if (job->callback != nullptr)
			{
				if (job->callback(event) == Propagate::Stop)
					break;
			}
			else
			{
				if (job->notify_callback() == Propagate::Stop)
					break;
			}
		}
		catch (...) {}
	}
}

void IMAF::Events::EventHandler::Run()
{
	{
		std::lock_guard m(m_mutex_queue);
		m_worker_thread_id = std::this_thread::get_id();
	}

	while (true)
	{
		Job job;
		{
			std::unique_lock<std::mutex> lc(m_mutex_queue);
			m_cv.wait(lc, [this]() { return !this->m_job_pool.empty() || m_end; });

			if (m_job_pool.empty())
			{
				if (m_end)
				{
					m_worker_thread_id = std::thread::id();
					m_worker_stopped = true;
					break;
				}
				continue;
			}

			job = std::move(m_job_pool.front());
			m_job_pool.pop();
		}

		if (std::holds_alternative<Event>(job))
		{
			ProcessEvent(std::get<Event>(job));
		}
		else
		{
			Unsub unsub = std::get<Unsub>(job);
			RemoveSub(unsub.sub_id);
			{
				std::lock_guard m(unsub.signal->mutex);
				unsub.signal->complete = true;
			}
			unsub.signal->cv.notify_one();
		}
	}
}