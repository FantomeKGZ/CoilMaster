#include "CM_WarehouseStore.h"

namespace CM
{
bool WarehouseStore::loadKnownWireDiameters(KnownWireDiameter* items,
                                             uint8_t capacity,
                                             uint8_t& count) const
{
    count = 0U;
    if (!ready() || items == nullptr || capacity == 0U)
    {
        return false;
    }

    for (uint8_t i = 0U; i < capacity; ++i)
    {
        items[i] = KnownWireDiameter();
    }

    if (!m_storage.exists(SpoolsPath)) return true;

    File file = m_storage.open(SpoolsPath, FILE_READ);
    if (!file) return false;

    uint32_t previousSpoolId = 0UL;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;

        uint32_t spoolId = 0UL;
        uint32_t diameter = 0UL;
        uint32_t weight = 0UL;
        String status;
        String wireType;
        String optional;

        const bool hasWireType = line.indexOf(F("\"wire_type\":")) >= 0;
        if (!line.startsWith("{") || !line.endsWith("}") ||
            !findUnsigned(line, "spool_id", spoolId) || spoolId == 0UL ||
            spoolId <= previousSpoolId ||
            !findUnsigned(line, "diameter_hundredths_mm", diameter) ||
            diameter == 0UL || diameter > 0xFFFFUL ||
            !findUnsigned(line, "current_weight_g", weight) ||
            !findString(line, "status", status) ||
            (hasWireType &&
             (!findString(line, "wire_type", wireType) ||
              (wireType != "CU" && wireType != "AL"))))
        {
            file.close();
            count = 0U;
            return false;
        }
        previousSpoolId = spoolId;

        const char* optionalKeys[] = {
            "manufacturer", "supplier", "batch", "storage_location", "comment"
        };
        for (uint8_t keyIndex = 0U;
             keyIndex < sizeof(optionalKeys) / sizeof(optionalKeys[0]);
             ++keyIndex)
        {
            const String marker = String("\"") + optionalKeys[keyIndex] + F("\":");
            if (line.indexOf(marker) >= 0 &&
                !findString(line, optionalKeys[keyIndex], optional))
            {
                file.close();
                count = 0U;
                return false;
            }
        }

        uint8_t index = count;
        for (uint8_t i = 0U; i < count; ++i)
        {
            if (items[i].diameterHundredthsMm == static_cast<uint16_t>(diameter))
            {
                index = i;
                break;
            }
        }

        if (index == count)
        {
            if (count >= capacity)
            {
                file.close();
                count = 0U;
                return false;
            }
            items[index].diameterHundredthsMm = static_cast<uint16_t>(diameter);
            ++count;
        }

        if (status == "ACTIVE")
        {
            if (items[index].availableGrams > 0xFFFFFFFFUL - weight)
            {
                file.close();
                count = 0U;
                return false;
            }
            items[index].availableGrams += weight;
        }
    }

    file.close();

    for (uint8_t i = 1U; i < count; ++i)
    {
        const KnownWireDiameter current = items[i];
        uint8_t position = i;
        while (position > 0U &&
               items[position - 1U].diameterHundredthsMm > current.diameterHundredthsMm)
        {
            items[position] = items[position - 1U];
            --position;
        }
        items[position] = current;
    }

    return true;
}

uint8_t WarehouseStore::loadKnownWireDiameters(KnownWireDiameter* items,
                                                uint8_t capacity) const
{
    uint8_t count = 0U;
    return loadKnownWireDiameters(items, capacity, count) ? count : 0U;
}
}
