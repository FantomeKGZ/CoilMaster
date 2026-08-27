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

    const char* optionalKeys[] = {
        "manufacturer", "supplier", "batch", "storage_location", "comment"
    };
    String optionalMarker;
    optionalMarker.reserve(24U);

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

        for (uint8_t keyIndex = 0U;
             keyIndex < sizeof(optionalKeys) / sizeof(optionalKeys[0]);
             ++keyIndex)
        {
            optionalMarker.remove(0);
            optionalMarker += '"';
            optionalMarker += optionalKeys[keyIndex];
            optionalMarker += F("\":");
            if (line.indexOf(optionalMarker) >= 0 &&
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

        const uint16_t targetDiameter = static_cast<uint16_t>(diameter);
        uint8_t lower = 0U;
        uint8_t upper = count;
        while (lower < upper)
        {
            const uint8_t middle = static_cast<uint8_t>(
                lower + static_cast<uint8_t>((upper - lower) / 2U));
            if (items[middle].diameterHundredthsMm < targetDiameter)
                lower = static_cast<uint8_t>(middle + 1U);
            else
                upper = middle;
        }

        uint8_t index = lower;
        if (index >= count || items[index].diameterHundredthsMm != targetDiameter)
        {
            if (count >= capacity)
            {
                file.close();
                count = 0U;
                return false;
            }
            for (uint8_t move = count; move > index; --move)
                items[move] = items[static_cast<uint8_t>(move - 1U)];
            items[index] = KnownWireDiameter();
            items[index].diameterHundredthsMm = targetDiameter;
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

    return true;
}
}
