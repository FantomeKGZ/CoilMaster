#ifndef CM_REPAIR_FINALIZATION_GUARD_H
#define CM_REPAIR_FINALIZATION_GUARD_H

#include <Arduino.h>
#include <FS.h>

namespace CM
{
enum class RepairFinalizationCheck : uint8_t
{
    Ready = 0,
    CostingStorageUnavailable = 1,
    CostingIntegrityFailed = 2,
    WindingStorageUnavailable = 3,
    WindingIntegrityFailed = 4,
    WireWriteOffRequired = 5,
    WireWriteOffStorageUnavailable = 6,
    WireWriteOffIntegrityFailed = 7
};

class RepairFinalizationGuard
{
public:
    static RepairFinalizationCheck check(fs::FS& storage, uint32_t repairId);
    // Read-only fast path for callers that have already completed one
    // authoritative repair-journal validation and exact-ID/open-state proof.
    static RepairFinalizationCheck checkKnownRepair(fs::FS& storage, uint32_t repairId);
};
}

#endif // CM_REPAIR_FINALIZATION_GUARD_H
