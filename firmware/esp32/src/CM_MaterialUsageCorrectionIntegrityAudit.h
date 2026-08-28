#ifndef CM_MATERIAL_USAGE_CORRECTION_INTEGRITY_AUDIT_H
#define CM_MATERIAL_USAGE_CORRECTION_INTEGRITY_AUDIT_H

#include <FS.h>

namespace CM
{
class MaterialUsageCorrectionIntegrityAudit
{
public:
    static bool check(fs::FS& storage);
};
}

#endif // CM_MATERIAL_USAGE_CORRECTION_INTEGRITY_AUDIT_H
