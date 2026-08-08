#include "CM_WarehouseStore.h"
#include "CM_WarehouseMovementIntegrityAudit.h"

namespace CM
{
bool WarehouseStore::confirmedWriteOffForSourceSession(uint32_t sourceSessionId,
                                                        bool& found) const
{
    found = false;
    if (!ready() || sourceSessionId == 0UL) return false;
    if (!WarehouseMovementIntegrityAudit::check(m_storage)) return false;
    if (!m_storage.exists(MovementsPath)) return true;

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

        const bool hasSource = line.indexOf(F("\"source_session_id\":")) >= 0;
        if (!hasSource) continue;

        uint32_t currentSource = 0UL;
        if (!findUnsigned(line, "source_session_id", currentSource) || currentSource == 0UL)
        {
            file.close();
            return false;
        }
        if (currentSource != sourceSessionId) continue;

        if (found)
        {
            file.close();
            return false;
        }
        found = true;
    }

    file.close();
    return true;
}
}
