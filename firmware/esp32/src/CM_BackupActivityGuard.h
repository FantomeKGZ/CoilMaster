#ifndef CM_BACKUP_ACTIVITY_GUARD_H
#define CM_BACKUP_ACTIVITY_GUARD_H

#include <Arduino.h>
#include <FS.h>

namespace CM
{
enum class BackupActivityCheck : uint8_t
{
    Safe = 0U,
    Busy,
    Unavailable
};

class BackupActivityGuard
{
public:
    static BackupActivityCheck check(fs::FS& storage);
};
}

#endif // CM_BACKUP_ACTIVITY_GUARD_H
