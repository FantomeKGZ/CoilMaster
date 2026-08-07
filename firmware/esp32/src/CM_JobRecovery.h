#pragma once

#include <Arduino.h>

#include "CM_JobStateStore.h"

namespace CM
{
enum class JobRecoveryDisposition : uint8_t
{
    None,
    RestoredForDisplay,
    ManualReviewRequired
};

struct JobRecoveryInfo
{
    JobRecoveryDisposition disposition;
    JobRuntimeState state;
    bool mayCreateNewJob;
    bool mayAutoQueue;
    bool mayAutoResume;

    JobRecoveryInfo();
};

class JobRecovery
{
public:
    static bool evaluate(const JobStateStore& store,
                         JobRecoveryInfo& recovery);

private:
    static bool requiresManualReview(const JobRuntimeState& state);
    static bool isTerminalDelivery(JobDeliveryState state);
};
}
