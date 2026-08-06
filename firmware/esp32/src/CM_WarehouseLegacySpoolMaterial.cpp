#include "CM_WarehouseStore.h"

namespace CM
{
bool WarehouseStore::assignLegacySpoolMaterial(uint32_t spoolId,
                                                const String& wireType)
{
    if (!m_ready || spoolId == 0UL ||
        (wireType != "CU" && wireType != "AL") ||
        !m_storage.exists(SpoolsPath))
    {
        return false;
    }

    File source = m_storage.open(SpoolsPath, FILE_READ);
    if (!source) return false;

    m_storage.remove(SpoolsTempPath);
    File target = m_storage.open(SpoolsTempPath, FILE_WRITE);
    if (!target)
    {
        source.close();
        return false;
    }

    bool found = false;
    bool valid = true;
    while (source.available())
    {
        String line = source.readStringUntil('\n');
        uint32_t currentId = 0UL;
        if (findUnsigned(line, "spool_id", currentId) && currentId == spoolId)
        {
            String existingMaterial;
            String status;
            findString(line, "wire_type", existingMaterial);
            findString(line, "status", status);
            if (existingMaterial.length() > 0U ||
                (status.length() > 0U && status != "ACTIVE"))
            {
                valid = false;
                break;
            }

            const int closing = line.lastIndexOf('}');
            if (closing < 0)
            {
                valid = false;
                break;
            }

            String enriched;
            enriched.reserve(line.length() + 24U);
            enriched = line.substring(0, closing);
            enriched += F(",\"wire_type\":\"");
            enriched += wireType;
            enriched += F("\"}");
            line = enriched;
            found = true;
        }

        if (target.println(line) == 0U)
        {
            valid = false;
            break;
        }
    }

    source.close();
    target.flush();
    target.close();

    if (!valid || !found)
    {
        m_storage.remove(SpoolsTempPath);
        return false;
    }

    m_storage.remove(SpoolsPath);
    return m_storage.rename(SpoolsTempPath, SpoolsPath);
}
}
