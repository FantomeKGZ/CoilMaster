#ifndef CM_CONDUCTOR_SETTINGS_INTEGRITY_AUDIT_H
#define CM_CONDUCTOR_SETTINGS_INTEGRITY_AUDIT_H

#include <FS.h>

namespace CM
{
class ConductorSettingsIntegrityAudit
{
public:
    static bool check(fs::FS& storage);
};
}

#endif // CM_CONDUCTOR_SETTINGS_INTEGRITY_AUDIT_H
