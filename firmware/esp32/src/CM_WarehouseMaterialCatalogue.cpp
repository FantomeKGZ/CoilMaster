#include "CM_WarehouseStore.h"

namespace CM
{
uint8_t WarehouseStore::loadKnownWireDiameters(const char* wireType,
                                                KnownWireDiameter* items,
                                                uint8_t capacity) const
{
    if (!m_ready || wireType == nullptr || items == nullptr || capacity == 0U ||
        !m_storage.exists(SpoolsPath))
    {
        return 0U;
    }

    String requiredType = wireType;
    requiredType.trim();
    requiredType.toUpperCase();
    if (requiredType != "CU" && requiredType != "AL") return 0U;

    for (uint8_t i = 0U; i < capacity; ++i) items[i] = KnownWireDiameter();

    File file = m_storage.open(SpoolsPath, FILE_READ);
    if (!file) return 0U;

    uint8_t count = 0U;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        String storedType;
        uint32_t diameter = 0UL;
        uint32_t weight = 0UL;
        String status;

        // Legacy records without wire_type are intentionally excluded.
        if (!findString(line, "wire_type", storedType) ||
            !findUnsigned(line, "diameter_hundredths_mm", diameter))
        {
            continue;
        }

        storedType.trim();
        storedType.toUpperCase();
        if (storedType != requiredType || diameter == 0UL || diameter > 500UL)
        {
            continue;
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
            if (count >= capacity) continue;
            items[count].diameterHundredthsMm = static_cast<uint16_t>(diameter);
            items[count].availableGrams = 0UL;
            ++count;
        }

        findString(line, "status", status);
        const bool active = status.length() == 0U || status == "ACTIVE";
        if (active && findUnsigned(line, "current_weight_g", weight))
        {
            const uint32_t current = items[index].availableGrams;
            items[index].availableGrams =
                weight > 0xFFFFFFFFUL - current ? 0xFFFFFFFFUL : current + weight;
        }
    }
    file.close();

    // Stable ascending order makes recommendations and API responses repeatable.
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

    return count;
}
}
