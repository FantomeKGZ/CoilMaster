#include "CM_WarehouseStore.h"

namespace CM
{
namespace
{
struct MaterialDiameterSummary
{
    uint16_t diameter;
    uint32_t remaining;
    uint16_t spoolCount;
    uint32_t consumedMonth;
    uint32_t consumedAll;

    MaterialDiameterSummary()
        : diameter(0U), remaining(0UL), spoolCount(0U),
          consumedMonth(0UL), consumedAll(0UL) {}
};

struct MaterialSummary
{
    const char* code;
    MaterialDiameterSummary diameters[WarehouseMaxDiameters];
    uint8_t count;
    uint32_t remaining;
    uint32_t spoolCount;
    uint32_t consumedMonth;
    uint32_t consumedAll;

    MaterialSummary()
        : code("UNKNOWN"), count(0U), remaining(0UL), spoolCount(0UL),
          consumedMonth(0UL), consumedAll(0UL) {}
};

uint8_t materialIndex(const String& value)
{
    if (value == "CU") return 0U;
    if (value == "AL") return 1U;
    return 2U;
}

MaterialDiameterSummary* findOrAdd(MaterialSummary& summary, uint16_t diameter)
{
    for (uint8_t i = 0U; i < summary.count; ++i)
        if (summary.diameters[i].diameter == diameter) return &summary.diameters[i];
    if (summary.count >= WarehouseMaxDiameters) return nullptr;
    MaterialDiameterSummary& item = summary.diameters[summary.count++];
    item.diameter = diameter;
    return &item;
}

void sortDiameters(MaterialSummary& summary)
{
    for (uint8_t i = 0U; i < summary.count; ++i)
        for (uint8_t j = static_cast<uint8_t>(i + 1U); j < summary.count; ++j)
            if (summary.diameters[j].diameter < summary.diameters[i].diameter)
            {
                const MaterialDiameterSummary temp = summary.diameters[i];
                summary.diameters[i] = summary.diameters[j];
                summary.diameters[j] = temp;
            }
}
}

bool WarehouseStore::appendMaterialSummaryJson(String& json,
                                                const char* monthPrefix) const
{
    if (!m_ready || monthPrefix == nullptr) return false;

    MaterialSummary groups[3];
    groups[0].code = "CU";
    groups[1].code = "AL";
    groups[2].code = "UNKNOWN";

    if (m_storage.exists(SpoolsPath))
    {
        File spools = m_storage.open(SpoolsPath, FILE_READ);
        if (!spools) return false;
        while (spools.available())
        {
            const String line = spools.readStringUntil('\n');
            uint32_t diameter = 0UL;
            uint32_t weight = 0UL;
            String status;
            String wireType;
            if (!findUnsigned(line, "diameter_hundredths_mm", diameter) ||
                !findUnsigned(line, "current_weight_g", weight)) continue;
            findString(line, "status", status);
            if (status.length() > 0U && status != "ACTIVE") continue;
            findString(line, "wire_type", wireType);
            MaterialSummary& group = groups[materialIndex(wireType)];
            MaterialDiameterSummary* item =
                findOrAdd(group, static_cast<uint16_t>(diameter));
            if (item == nullptr) continue;
            item->remaining += weight;
            if (item->spoolCount < 0xFFFFU) ++item->spoolCount;
            group.remaining += weight;
            ++group.spoolCount;
        }
        spools.close();
    }

    if (m_storage.exists(MovementsPath))
    {
        File movements = m_storage.open(MovementsPath, FILE_READ);
        if (!movements) return false;
        while (movements.available())
        {
            const String line = movements.readStringUntil('\n');
            String type;
            String status;
            String timestamp;
            String wireType;
            uint32_t spoolId = 0UL;
            uint32_t diameter = 0UL;
            uint32_t grams = 0UL;
            if (!findString(line, "type", type) || type != "WRITE_OFF" ||
                !findString(line, "status", status) || status != "CONFIRMED" ||
                !findUnsigned(line, "diameter_hundredths_mm", diameter) ||
                !findUnsigned(line, "mass_g", grams)) continue;

            if (!findString(line, "wire_type", wireType) &&
                findUnsigned(line, "spool_id", spoolId) &&
                m_storage.exists(SpoolsPath))
            {
                File spools = m_storage.open(SpoolsPath, FILE_READ);
                if (!spools) { movements.close(); return false; }
                while (spools.available())
                {
                    const String spoolLine = spools.readStringUntil('\n');
                    uint32_t currentId = 0UL;
                    if (findUnsigned(spoolLine, "spool_id", currentId) &&
                        currentId == spoolId)
                    {
                        findString(spoolLine, "wire_type", wireType);
                        break;
                    }
                }
                spools.close();
            }

            MaterialSummary& group = groups[materialIndex(wireType)];
            MaterialDiameterSummary* item =
                findOrAdd(group, static_cast<uint16_t>(diameter));
            if (item == nullptr) continue;
            item->consumedAll += grams;
            group.consumedAll += grams;
            if (findString(line, "timestamp", timestamp) &&
                timestamp.startsWith(monthPrefix))
            {
                item->consumedMonth += grams;
                group.consumedMonth += grams;
            }
        }
        movements.close();
    }

    for (uint8_t i = 0U; i < 3U; ++i) sortDiameters(groups[i]);

    json += F("\"materials\":[");
    for (uint8_t groupIndex = 0U; groupIndex < 3U; ++groupIndex)
    {
        if (groupIndex > 0U) json += ',';
        const MaterialSummary& group = groups[groupIndex];
        json += F("{\"material\":\""); json += group.code;
        json += F("\",\"remaining_g\":"); json += group.remaining;
        json += F(",\"active_spools\":"); json += group.spoolCount;
        json += F(",\"consumed_month_g\":"); json += group.consumedMonth;
        json += F(",\"consumed_all_time_g\":"); json += group.consumedAll;
        json += F(",\"diameter_count\":"); json += group.count;
        json += F(",\"diameters\":[");
        for (uint8_t i = 0U; i < group.count; ++i)
        {
            if (i > 0U) json += ',';
            const MaterialDiameterSummary& item = group.diameters[i];
            json += F("{\"diameter_hundredths_mm\":"); json += item.diameter;
            json += F(",\"remaining_g\":"); json += item.remaining;
            json += F(",\"active_spools\":"); json += item.spoolCount;
            json += F(",\"consumed_month_g\":"); json += item.consumedMonth;
            json += F(",\"consumed_all_time_g\":"); json += item.consumedAll;
            json += '}';
        }
        json += F("]}");
    }
    json += F("]");
    return true;
}
}
