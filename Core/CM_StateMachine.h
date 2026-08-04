#ifndef CM_STATE_MACHINE_H
#define CM_STATE_MACHINE_H

#include <stdint.h>

#include "CM_Types.h"
#include "CM_WindingJob.h"

namespace CM
{
class StateMachine
{
public:
    StateMachine();

    void resetToHome();

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
    bool cancel();
    void setFault();

private:
    void finishActiveCoil();

    MachineState m_state;
    WindingJob m_job;
};
}

#endif // CM_STATE_MACHINE_H
