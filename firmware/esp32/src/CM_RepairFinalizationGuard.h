#ifndef CM_REPAIR_FINALIZATION_GUARD_H
#define CM_REPAIR_FINALIZATION_GUARD_H

#include <Arduino.h>
#include <FS.h>

namespace CM
{
enum class RepairFinalizationCheck : uint8_t
{
    Ready = 0,
    StorageUnavailable = 1,
    IntegrityFailed = 2
};

class RepairFinalizationGuard
{
public:
    static RepairFinalizationCheck check(fs::FS& storage, uint32_t repairId);
};
}

#endif // CM_REPAIR_FINALIZATION_GUARD_H
