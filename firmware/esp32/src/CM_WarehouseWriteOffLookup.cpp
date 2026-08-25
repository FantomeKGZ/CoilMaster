#include "CM_WarehouseStore.h"
#include "CM_WarehouseMovementIntegrityAudit.h"

namespace CM
{
bool WarehouseStore::confirmedWriteOffForSourceRun(uint32_t sourceSessionId,
                                                    uint32_t sourceRunId,
                                                    bool& found) const
{
    found = false;
    if (!ready() || sourceSessionId == 0UL || sourceRunId == 0UL) return false;
    if (!m_storage.exists(MovementsPath)) return true;

    // Keep duplicate protection fail-closed. The authoritative movement audit
    // validates transaction pairing and provenance uniqueness while resolving
    // the requested exact run during the same primary file pass.
    return WarehouseMovementIntegrityAudit::checkSourceRun(
        m_storage, sourceSessionId, sourceRunId, found);
}
}
