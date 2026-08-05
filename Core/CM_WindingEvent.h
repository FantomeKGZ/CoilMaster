#ifndef CM_WINDING_EVENT_H
#define CM_WINDING_EVENT_H

#include <stdint.h>

namespace CM
{
enum class WindingEventType : uint8_t
{
    None = 0U,
    RunStarted,
    RunCompleted
};

struct WindingEvent
{
    WindingEventType type;
    uint32_t sessionId;
    uint32_t runId;
    uint16_t completedRuns;

    WindingEvent()
        : type(WindingEventType::None),
          sessionId(0UL),
          runId(0UL),
          completedRuns(0U)
    {
    }
};
}

#endif // CM_WINDING_EVENT_H
