#pragma once

#include <Arduino.h>

#include "CM_JobSnapshotStore.h"

namespace CM
{
enum class JobLinkageRequestResult : uint8_t
{
    Unlinked,
    Linked,
    Partial,
    Invalid
};

class JobLinkageRequest
{
public:
    // Parses optional repair_id and motor_id request values.
    // Both absent means an explicitly unlinked job. Both present must be
    // canonical non-zero uint32 decimal strings. Partial linkage is rejected.
    static JobLinkageRequestResult parse(bool hasRepairId,
                                         const String& repairIdText,
                                         bool hasMotorId,
                                         const String& motorIdText,
                                         JobLinkage& linkage);

private:
    static bool parseCanonicalId(const String& text, uint32_t& value);
};
}
