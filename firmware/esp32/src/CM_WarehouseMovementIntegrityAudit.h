#pragma once

#include <FS.h>

namespace CM
{
class WarehouseMovementIntegrityAudit
{
public:
    static bool check(fs::FS& storage);
};
}
