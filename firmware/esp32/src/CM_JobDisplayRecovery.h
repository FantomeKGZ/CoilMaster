#pragma once

#include <Arduino.h>

#include "CM_JobSnapshotStore.h"

namespace CM
{
struct RecoveredJobDisplay
{
    uint32_t jobId;
    uint32_t sessionId;
    RemoteJobType type;
    uint8_t coilCount;
    uint16_t turns[10];
    JobLinkage linkage;

    RecoveredJobDisplay();
};

class JobDisplayRecovery
{
public:
    // Loads only immutable, validated data for UI/API display after reboot.
    // This helper never queues UART work and never changes machine outputs.
    static bool load(const JobSnapshotStore& snapshots,
                     uint32_t expectedJobId,
                     uint32_t expectedSessionId,
                     RecoveredJobDisplay& display);
};
}
