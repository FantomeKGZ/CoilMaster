#ifndef CM_BACKUP_BUSINESS_DATA_INTEGRITY_AUDIT_H
#define CM_BACKUP_BUSINESS_DATA_INTEGRITY_AUDIT_H

#include <FS.h>

namespace CM
{
class BackupBusinessDataIntegrityAudit
{
public:
    static bool check(fs::FS& storage);
};
}

#endif // CM_BACKUP_BUSINESS_DATA_INTEGRITY_AUDIT_H
