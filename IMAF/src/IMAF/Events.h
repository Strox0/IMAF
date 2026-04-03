#pragma once
#include <cstdint>
#include <any>
#include <functional>
#include <string_view>

namespace IMAF
{
	namespace Events
	{
		using EventID = uint32_t;

		struct Event
		{
			EventID event_id{};
			std::any data{};
		};

		//return true if the event should not be propagated further
		using EventCallback = bool (*)(const Event&);

		struct EventSubID
		{
			EventID event_id;
			EventCallback sub_id = 0;

			bool operator!()
			{
				return sub_id == 0;
			}
		};

		void Dispatch(const Event& event);

		EventSubID Subscribe(EventID event_id, EventCallback callback);

		void Unsubscribe(EventSubID sub_id);

		void RegisterEvent(EventID event_id);
	}
}