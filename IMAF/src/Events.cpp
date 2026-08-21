#include "IMAF/Events.h"
#include "IMAF/EventHandler.h"
#include <any>

void IMAF::Events::Dispatch(const Event& event)
{
	IMAF::Events::EventHandler::Get().Dispatch(event);
}

void IMAF::Events::Dispatch(EventID event_id, std::any data)
{
	Dispatch(IMAF::Events::Event{ event_id,data });
}

void IMAF::Events::Notify(EventID event_id)
{
	IMAF::Events::EventHandler::Get().Notify(event_id);
}

IMAF::Events::EventSubID IMAF::Events::Subscribe(EventID event_id, EventCallback callback)
{
	return IMAF::Events::EventHandler::Get().Subscribe(event_id, callback);
}

IMAF::Events::EventSubID IMAF::Events::Subscribe(EventID event_id, NotifyCallback callback)
{
	return IMAF::Events::EventHandler::Get().Subscribe(event_id, callback);
}

void IMAF::Events::Unsubscribe(EventSubID sub_id)
{
	IMAF::Events::EventHandler::Get().Unsubscribe(sub_id);
}