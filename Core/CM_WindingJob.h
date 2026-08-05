#ifndef CM_WINDING_JOB_H
#define CM_WINDING_JOB_H

#include <stdint.h>

#include "CM_Types.h"

namespace CM
{
struct WindingJob
{
    uint32_t jobId;
    uint32_t sessionId;
    uint32_t currentRunId;
    WindingType type;
    JobSource source;
    JobStatus status;
    uint8_t coilCount;
    uint8_t currentCoil;
    uint16_t currentTurns;
    uint16_t completedRuns;
    uint16_t targetTurns[MaxCoilsPerJob];

    void clear()
    {
        jobId = 0UL;
        sessionId = 0UL;
        currentRunId = 0UL;
        type = WindingType::Working;
        source = JobSource::LocalKeypad;
        status = JobStatus::Empty;
        coilCount = 0U;
        currentCoil = 0U;
        currentTurns = 0U;
        completedRuns = 0U;

        for (uint8_t index = 0U; index < MaxCoilsPerJob; ++index)
        {
            targetTurns[index] = 0U;
        }
    }

    bool isValid() const
    {
        if (coilCount == 0U || coilCount > MaxCoilsPerJob)
        {
            return false;
        }

        for (uint8_t index = 0U; index < coilCount; ++index)
        {
            if (targetTurns[index] == 0U ||
                targetTurns[index] > MaxTurnsPerCoil)
            {
                return false;
            }
        }

        return true;
    }

    uint16_t activeTarget() const
    {
        return currentCoil < coilCount ? targetTurns[currentCoil] : 0U;
    }

    bool hasMoreCoils() const
    {
        return currentCoil < coilCount;
    }
};
}

#endif // CM_WINDING_JOB_H
