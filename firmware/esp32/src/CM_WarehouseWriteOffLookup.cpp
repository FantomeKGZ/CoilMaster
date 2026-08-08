#include "CM_WarehouseStore.h"
#include "CM_WarehouseMovementIntegrityAudit.h"

namespace CM
{
bool WarehouseStore::confirmedWriteOffForSourceSession(uint32_t sourceSessionId,
                                                        bool& found) const
{
    found = false;
    if (!ready() || sourceSessionId == 0UL) return false;
    if (!m_storage.exists(MovementsPath)) return true;

    // The lookup is only meaningful on an authoritative, unambiguous movement
    // history. In particular this rejects malformed transactions and duplicate
    // legacy/run-level provenance before returning a positive match.
    if (!WarehouseMovementIntegrityAudit::check(m_storage)) return false;

    File file = m_storage.open(MovementsPath, FILE_READ);
    if (!file || file.isDirectory())
    {
        if (file) file.close();
        return false;
    }

    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;

        String status;
        if (!findString(line, "status", status))
        {
            file.close();
            return false;
        }
        if (status != "CONFIRMED") continue;
        if (line.indexOf(F("\"source_session_id\":")) < 0) continue;

        uint32_t currentSessionId = 0UL;
        if (!findUnsigned(line, "source_session_id", currentSessionId) ||
            currentSessionId == 0UL)
        {
            file.close();
            return false;
        }
        if (currentSessionId == sourceSessionId) found = true;
    }

    file.close();
    return true;
}

bool WarehouseStore::confirmedWriteOffForSourceRun(uint32_t sourceSessionId,
                                                    uint32_t sourceRunId,
                                                    bool& found) const
{
    found = false;
    if (!ready() || sourceSessionId == 0UL || sourceRunId == 0UL) return false;
    if (!m_storage.exists(MovementsPath)) return true;

    // Keep duplicate protection fail-closed. The authoritative movement audit
    // validates PENDING -> CONFIRMED|ABORTED pairing and provenance uniqueness;
    // this second pass only resolves the requested exact run.
    if (!WarehouseMovementIntegrityAudit::check(m_storage)) return false;

    File file = m_storage.open(MovementsPath, FILE_READ);
    if (!file || file.isDirectory())
    {
        if (file) file.close();
        return false;
    }

    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;

        String status;
        if (!findString(line, "status", status))
        {
            file.close();
            return false;
        }
        if (status != "CONFIRMED") continue;

        const bool hasSession = line.indexOf(F("\"source_session_id\":")) >= 0;
        const bool hasRun = line.indexOf(F("\"source_run_id\":")) >= 0;
        if (!hasSession || !hasRun) continue;

        uint32_t currentSessionId = 0UL;
        uint32_t currentRunId = 0UL;
        if (!findUnsigned(line, "source_session_id", currentSessionId) ||
            currentSessionId == 0UL ||
            !findUnsigned(line, "source_run_id", currentRunId) ||
            currentRunId == 0UL)
        {
            file.close();
            return false;
        }

        if (currentSessionId == sourceSessionId && currentRunId == sourceRunId)
            found = true;
    }

    file.close();
    return true;
}
}
