#include "IMAF/Events.h"
#include "IMAF/EventHandler.h"

void IMAF::Events::Dispatch(const Event& event)
{
	IMAF::Events::EventHandler::Get().Dispatch(event);
}

IMAF::Events::EventSubID IMAF::Events::Subscribe(EventID event_id, EventCallback callback)
{
	return IMAF::Events::EventHandler::Get().Subscribe(event_id, callback);
}

void IMAF::Events::Unsubscribe(EventSubID sub_id)
{
	IMAF::Events::EventHandler::Get().Unsubscribe(sub_id);
}

void IMAF::Events::RegisterEvent(EventID event_id)
{
	IMAF::Events::EventHandler::Get().RegisterEvent(event_id);
}
