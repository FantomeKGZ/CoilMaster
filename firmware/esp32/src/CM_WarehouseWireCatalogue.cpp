#include "CM_WarehouseStore.h"

namespace CM
{
uint8_t WarehouseStore::loadKnownWireDiameters(KnownWireDiameter* items,
                                                uint8_t capacity) const
{
    if (!m_ready || items == nullptr || capacity == 0U ||
        !m_storage.exists(SpoolsPath))
    {
        return 0U;
    }

    for (uint8_t i = 0U; i < capacity; ++i)
    {
        items[i] = KnownWireDiameter();
    }

    File file = m_storage.open(SpoolsPath, FILE_READ);
    if (!file)
    {
        return 0U;
    }

    uint8_t count = 0U;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        uint32_t diameter = 0UL;
        uint32_t weight = 0UL;
        String status;

        if (!findUnsigned(line, "diameter_hundredths_mm", diameter) ||
            diameter == 0UL || diameter > 0xFFFFUL)
        {
            continue;
        }

        findUnsigned(line, "current_weight_g", weight);
        findString(line, "status", status);

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
                continue;
            }
            items[index].diameterHundredthsMm = static_cast<uint16_t>(diameter);
            ++count;
        }

        if (status.length() == 0U || status == "ACTIVE")
        {
            const uint64_t total = static_cast<uint64_t>(items[index].availableGrams) + weight;
            items[index].availableGrams = total > 0xFFFFFFFFULL
                                              ? 0xFFFFFFFFUL
                                              : static_cast<uint32_t>(total);
        }
    }

    file.close();

    // Stable ascending order makes API output and calculations deterministic.
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

    return count;
}
}
