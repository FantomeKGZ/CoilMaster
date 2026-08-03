/*
==========================================================
CoilMaster OS
CM_Event
==========================================================
*/

#include "CM_Event.h"
#include "CM_Logger.h"

CM_EventID CM_Event::_lastEvent = CM_EVENT_NONE;

void CM_Event::begin()
{
    _lastEvent = CM_EVENT_NONE;
}

void CM_Event::send(CM_EventID event)
{
    _lastEvent = event;

    switch(event)
    {
        case CM_EVENT_SYSTEM_BOOT:
            CM_Logger::info("EVENT_SYSTEM_BOOT");
            break;

        case CM_EVENT_SYSTEM_READY:
            CM_Logger::info("EVENT_SYSTEM_READY");
            break;

        case CM_EVENT_HALL_TRIGGER:
            CM_Logger::debug("EVENT_HALL_TRIGGER");
            break;

        case CM_EVENT_WINDING_START:
            CM_Logger::info("EVENT_WINDING_START");
            break;

        case CM_EVENT_WINDING_STOP:
            CM_Logger::info("EVENT_WINDING_STOP");
            break;

        case CM_EVENT_WINDING_FINISH:
            CM_Logger::info("EVENT_WINDING_FINISH");
            break;

        case CM_EVENT_ERROR:
            CM_Logger::error("EVENT_ERROR");
            break;

        default:
            break;
    }
}

CM_EventID CM_Event::lastEvent()
{
    return _lastEvent;
}