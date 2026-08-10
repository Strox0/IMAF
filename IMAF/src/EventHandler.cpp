#include "IMAF/EventHandler.h"
#include "IMAF/Events.h"
#include <limits>
#include <mutex>
#include <utility>
#include <vector>
#include <exception>

IMAF::Events::EventHandler::EventHandler() : m_t(&IMAF::Events::EventHandler::Run, this), m_random(1,std::numeric_limits<IMAF::Events::SubID>::max())
{
	
};

IMAF::Events::EventHandler::~EventHandler()
{
	End();
}

// ISSUE 5: Manual lock/unlock pairs surround throwing operations here and in related event and panel paths, so exceptions can permanently strand mutexes.
//FIXED
void IMAF::Events::EventHandler::Dispatch(const Event& event)
{
	{
		std::lock_guard<std::mutex> m(m_mutex_queue);
		m_event_pool.emplace(event);
	}

	m_cv.notify_one();
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
			m_events_map.emplace(event_id, std::vector<EventPair>());
		}

		m_events_map[event_id].emplace_back(EventPair(callback, id));
	}

	EventSubID subid;
	subid.event_id = event_id;
	subid.sub_id = id;
	
	return subid;
}

void IMAF::Events::EventHandler::Unsubscribe(EventSubID sub_id)
{
	std::lock_guard me(m_mutex_events);
	if (!m_events_map.contains(sub_id.event_id))
		return;

	auto it = m_events_map[sub_id.event_id].begin();
	while (it != m_events_map[sub_id.event_id].end())
	{
		if (it->sub_id != sub_id.sub_id)
		{
			it++;
			continue;
		}
		
		break;
	}

	if (it != m_events_map[sub_id.event_id].end() && it->sub_id == sub_id.sub_id)
	{
		m_events_map[sub_id.event_id].erase(it);

		{
			std::lock_guard mids(m_mutex_ids);
			m_ids.erase(sub_id.sub_id);
		}
	}
}

// ISSUE 29: End can self-join when called from a callback, and shutdown does not define whether queued events are drained or discarded.
//FIXED
void IMAF::Events::EventHandler::End()
{
	m_end = true;
	m_cv.notify_all();
	if (m_t.joinable())
		m_t.join();
}

// ISSUE 4: Exceptions escaping an event callback also escape the worker thread entry point and invoke std::terminate.
//FIXED
void IMAF::Events::EventHandler::Run()
{
	while (!m_end)
	{
		Event event;
		{
			std::unique_lock<std::mutex> lc(m_mutex_queue);
			m_cv.wait(lc, [this]() { return !this->m_event_pool.empty() || m_end; });

			if (m_event_pool.empty())
				continue;

			m_mutex_events.lock();
			event = std::move(m_event_pool.front());
			m_event_pool.pop();
		}

		if (!m_events_map.contains(event.event_id))
		{
			m_mutex_events.unlock();
			continue;
		}
		const std::vector<EventPair> callbacks = m_events_map[event.event_id];
		m_mutex_events.unlock();
		for (const auto& [cb,id] : callbacks)
		{
			try
			{
				if (cb(event) == Propagate::Stop)
					break;
			}
			catch (std::exception& e)
			{

			}
		}
	}
}