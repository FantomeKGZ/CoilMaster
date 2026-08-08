#pragma once

#include <Arduino.h>
#include <FS.h>

namespace CM
{
enum class WireWriteOffCoverageCheck : uint8_t
{
    Covered,
    WriteOffRequired,
    StorageUnavailable,
    IntegrityFailed
};

class WireWriteOffCoverageAudit
{
public:
    static WireWriteOffCoverageCheck check(fs::FS& storage,
                                           uint32_t repairId);
};
}
