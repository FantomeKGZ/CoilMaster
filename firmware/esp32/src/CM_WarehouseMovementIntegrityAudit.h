#pragma once

#include <FS.h>
#include <stdint.h>

namespace CM
{
class WarehouseMovementIntegrityAudit
{
public:
    static bool check(fs::FS& storage);
    static bool check(fs::FS& storage, uint32_t& validatedRecordCount);
};
}
