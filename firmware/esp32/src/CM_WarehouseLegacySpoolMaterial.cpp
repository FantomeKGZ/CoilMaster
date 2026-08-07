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
    uint32_t previousId = 0UL;
    while (source.available())
    {
        String line = source.readStringUntil('\n');
        if (line.length() == 0U) continue;

        uint32_t currentId = 0UL;
        uint32_t diameter = 0UL;
        uint32_t weight = 0UL;
        String status;
        if (!findUnsigned(line, "spool_id", currentId) || currentId == 0UL ||
            currentId <= previousId ||
            !findUnsigned(line, "diameter_hundredths_mm", diameter) ||
            diameter == 0UL || diameter > 0xFFFFUL ||
            !findUnsigned(line, "current_weight_g", weight) ||
            !findString(line, "status", status))
        {
            valid = false;
            break;
        }
        previousId = currentId;

        const bool hasMaterial = line.indexOf(F("\"wire_type\":")) >= 0;
        String existingMaterial;
        if (hasMaterial &&
            (!findString(line, "wire_type", existingMaterial) ||
             (existingMaterial != "CU" && existingMaterial != "AL")))
        {
            valid = false;
            break;
        }

        if (currentId == spoolId)
        {
            if (found || hasMaterial || status != "ACTIVE")
            {
                valid = false;
                break;
            }

            const int closing = line.lastIndexOf('}');
            if (closing < 0 || closing != line.length() - 1)
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

            String verified;
            if (!findString(line, "wire_type", verified) || verified != wireType)
            {
                valid = false;
                break;
            }
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

    return replaceSpoolsFileFromTemp();
}
}
