#include "CM_WarehouseStore.h"

namespace CM
{
bool WarehouseStore::loadActiveSpoolIdentity(uint32_t spoolId,
                                              ActiveWireSpoolIdentity& identity) const
{
    bool found = false;
    return loadActiveSpoolIdentity(spoolId, identity, found) && found;
}

bool WarehouseStore::loadActiveSpoolIdentity(uint32_t spoolId,
                                              ActiveWireSpoolIdentity& identity,
                                              bool& found) const
{
    identity = ActiveWireSpoolIdentity();
    found = false;
    if (!ready() || spoolId == 0UL) return false;
    if (!m_storage.exists(SpoolsPath)) return true;

    File file = m_storage.open(SpoolsPath, FILE_READ);
    if (!file || file.isDirectory())
    {
        if (file) file.close();
        return false;
    }

    uint32_t previousId = 0UL;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
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
            file.close();
            return false;
        }
        previousId = currentId;

        String wireType;
        const bool hasWireType = line.indexOf(F("\"wire_type\":")) >= 0;
        if (hasWireType &&
            (!findString(line, "wire_type", wireType) ||
             (wireType != "CU" && wireType != "AL")))
        {
            file.close();
            return false;
        }

        const char* optionalFields[] = {
            "manufacturer", "supplier", "batch", "storage_location", "comment"
        };
        for (uint8_t index = 0U; index < 5U; ++index)
        {
            const String marker = String("\"") + optionalFields[index] + F("\":");
            if (line.indexOf(marker) >= 0)
            {
                String value;
                if (!findString(line, optionalFields[index], value))
                {
                    file.close();
                    return false;
                }
            }
        }

        if (currentId != spoolId) continue;
        if (found)
        {
            file.close();
            return false;
        }
        found = true;
        if (status != "ACTIVE" || weight == 0UL ||
            (wireType != "CU" && wireType != "AL"))
        {
            identity = ActiveWireSpoolIdentity();
            continue;
        }

        identity.spoolId = currentId;
        identity.diameterHundredthsMm = static_cast<uint16_t>(diameter);
        identity.currentWeightGrams = weight;
        identity.wireType = wireType;
    }
    file.close();

    if (!found) return true;
    if (identity.spoolId == 0UL)
    {
        found = false;
        return true;
    }
    return true;
}
}
