#ifndef CM_STATE_MACHINE_H
#define CM_STATE_MACHINE_H

#include <stdint.h>

#include "CM_Types.h"
#include "CM_WindingEvent.h"
#include "CM_WindingJob.h"

namespace CM
{
class StateMachine
{
public:
    StateMachine();

    void resetToHome();
    bool returnHome();

    MachineState state() const;
    const WindingJob& job() const;
    WindingJob& job();

    bool setCoilCount(uint8_t count);
    bool setCoilTurns(uint8_t index, uint16_t turns);
    bool loadRemoteJob(const WindingJob& remoteJob);

    bool startOrResume();
    bool pause();
    bool toggleManual();
    bool registerTurn();
    bool acknowledgeCoilComplete();
    bool acknowledgeDeliveredRun(uint32_t runId);
    bool cancel();
    void setFault();

    bool takeEvent(WindingEvent& event);

    void setNextIdentifiers(uint32_t nextSessionId, uint32_t nextRunId);
    uint32_t nextSessionId() const;
    uint32_t nextRunId() const;

private:
    bool beginRun();
    void finishActiveCoil();
    void publishEvent(WindingEventType type);
    uint32_t allocateSessionId();
    uint32_t allocateRunId();

    MachineState m_state;
    WindingJob m_job;
    WindingEvent m_pendingEvent;
    bool m_hasPendingEvent;
    uint32_t m_nextSessionId;
    uint32_t m_nextRunId;
};
}

#endif // CM_STATE_MACHINE_H
