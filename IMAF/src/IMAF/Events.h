#pragma once
#include <cstdint>
#include <any>
#include <functional>
#include <expected>

namespace IMAF
{
	namespace Events
	{
		using EventID = uint32_t;
		using SubID = uint64_t;

		struct Event
		{
			EventID event_id{};
			std::any data{};
		};

		enum Propagate
		{
			Stop,
			Continue
		};

		using EventCallback = std::function<Propagate(const Event&)>;

		struct EventSubID
		{
			EventID event_id = -1;
			SubID sub_id = 0;

			operator bool() const
			{
				return sub_id != 0;
			}
		};

		//Non Blocking
		void Dispatch(const Event& event);
		void Dispatch(EventID event_id, std::any data);

		//Order is call order
		EventSubID Subscribe(EventID event_id, EventCallback callback);

		void Unsubscribe(EventSubID sub_id);
		
		enum DataError
		{
			IncorrectID,
			IncorrectType
		};

		// ISSUE 13: GetEventData declares bool as its error type but returns DataError values, collapsing the two error cases to false and true.
		//FIXED
		template<typename T>
		std::expected<T,DataError> GetEventData(const Event& data, EventID expected)
		{
			if (data.event_id != expected)
				return std::unexpected(DataError::IncorrectID);

			if (data.data.type() != typeid(T))
				return std::unexpected(DataError::IncorrectType);

			return std::any_cast<T>(data.data);
		}
	}
}