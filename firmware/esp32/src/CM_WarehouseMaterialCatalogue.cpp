#include "CM_WarehouseStore.h"
#include "CM_FlatJsonObjectValidator.h"

namespace CM
{
bool WarehouseStore::loadKnownWireDiameters(const char* wireType,
                                             KnownWireDiameter* items,
                                             uint8_t capacity,
                                             uint8_t& count) const
{
    count = 0U;
    if (!ready() || wireType == nullptr || items == nullptr || capacity == 0U)
    {
        return false;
    }

    String requiredType = wireType;
    requiredType.trim();
    requiredType.toUpperCase();
    if (requiredType != "CU" && requiredType != "AL") return false;

    for (uint8_t i = 0U; i < capacity; ++i) items[i] = KnownWireDiameter();
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
        String storedType;
        String optional;

        const bool hasWireType = line.indexOf(F("\"wire_type\":")) >= 0;
        if (!FlatJsonObjectValidator::valid(line) ||
            !findUnsigned(line, "spool_id", spoolId) || spoolId == 0UL ||
            spoolId <= previousSpoolId ||
            !findUnsigned(line, "diameter_hundredths_mm", diameter) ||
            diameter == 0UL || diameter > 500UL ||
            !findUnsigned(line, "current_weight_g", weight) ||
            !findString(line, "status", status) ||
            (hasWireType &&
             (!findString(line, "wire_type", storedType) ||
              (storedType != "CU" && storedType != "AL"))))
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

        // Legacy records without wire_type are structurally valid but intentionally
        // excluded from material-specific recommendations.
        if (!hasWireType || storedType != requiredType) continue;

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

    for (uint8_t i = 0U; i < count; ++i)
    {
        for (uint8_t j = static_cast<uint8_t>(i + 1U); j < count; ++j)
        {
            if (items[j].diameterHundredthsMm < items[i].diameterHundredthsMm)
            {
                const KnownWireDiameter temporary = items[i];
                items[i] = items[j];
                items[j] = temporary;
            }
        }
    }

    return true;
}

uint8_t WarehouseStore::loadKnownWireDiameters(const char* wireType,
                                                KnownWireDiameter* items,
                                                uint8_t capacity) const
{
    uint8_t count = 0U;
    return loadKnownWireDiameters(wireType, items, capacity, count) ? count : 0U;
}
}
