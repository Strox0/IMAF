#include "IMAF/EventHandler.h"
#include <numeric>
#include <algorithm>

struct pair_pred
{
	constexpr bool operator()(const IMAF::Events::_EventPair& a, const IMAF::Events::_EventPair& b) const
	{
		return a.sub_id < b.sub_id;
	}
};

IMAF::Events::EventHandler::EventHandler() : m_t(&IMAF::Events::EventHandler::Run, this), m_random(1,std::numeric_limits<IMAF::Events::SubID>::max())
{
	
};

IMAF::Events::EventHandler::~EventHandler()
{
	m_end = true;
	m_t.join();
}

void IMAF::Events::EventHandler::Dispatch(const Event& event)
{
	m_mutex_queue.lock();
	m_event_pool.emplace(event);
	m_mutex_queue.unlock();

	m_cv.notify_one();
}

IMAF::Events::EventSubID IMAF::Events::EventHandler::Subscribe(EventID event_id, EventCallback callback)
{
	m_mutex_ids.lock();

	SubID id = 0;
	do
	{
		id = m_random.Gen();
	} while (m_ids.contains(id));
	m_ids.insert(id);

	m_mutex_ids.unlock();

	m_mutex_events.lock();

	if (!m_events_map.contains(event_id))
	{
		m_events_map.emplace(event_id, std::vector<_EventPair>());
	}

	auto it = std::lower_bound(m_events_map[event_id].begin(), m_events_map[event_id].end(), _EventPair(nullptr,id), pair_pred{});
	m_events_map[event_id].insert(it, { callback, id });

	m_mutex_events.unlock();

	EventSubID subid;
	subid.event_id = event_id;
	subid.sub_id = id;
	
	return subid;
}

void IMAF::Events::EventHandler::Unsubscribe(EventSubID sub_id)
{
	m_mutex_events.lock();
	if (!m_events_map.contains(sub_id.event_id))
	{
		m_mutex_events.unlock();
		return;
	}
	
	auto it = std::lower_bound(m_events_map[sub_id.event_id].begin(), m_events_map[sub_id.event_id].end(), _EventPair(nullptr,sub_id.sub_id), pair_pred{});

	if (it != m_events_map[sub_id.event_id].end() && it->sub_id == sub_id.sub_id)
		m_events_map[sub_id.event_id].erase(it);

	m_mutex_events.unlock();

	m_mutex_ids.lock();
	m_ids.erase(sub_id.sub_id);
	m_mutex_ids.unlock();
}

void IMAF::Events::EventHandler::End()
{
	m_end = true;
	m_cv.notify_all();
}

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

			event = std::move(m_event_pool.front());
			m_event_pool.pop();
		}

		m_mutex_events.lock();
		const std::vector<_EventPair> callbacks = m_events_map[event.event_id];
		m_mutex_events.unlock();
		for (const auto& [callback,id] : callbacks)
		{
			if (callback(event) == Propagate::Stop)
				break;
		}
	}
}