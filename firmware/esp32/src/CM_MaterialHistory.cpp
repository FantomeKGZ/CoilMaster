#include "CM_MaterialLedger.h"

namespace CM
{
bool MaterialLedger::appendAdjustmentHistoryJson(String& json,
                                                 uint32_t materialId,
                                                 uint16_t limit,
                                                 uint16_t& count) const
{
    count = 0U;
    if (!m_ready) return false;
    if (!m_storage.exists(AdjustmentsPath)) return true;
    if (limit == 0U) limit = 20U;

    File file = m_storage.open(AdjustmentsPath, FILE_READ);
    if (!file) return false;

    bool first = true;
    while (file.available() && count < limit)
    {
        const String line = file.readStringUntil('\n');
        uint32_t currentMaterialId = 0UL;
        String status;
        if (!findUnsigned(line, "material_id", currentMaterialId)) continue;
        findString(line, "status", status);
        if (status.length() > 0U && status != "CONFIRMED") continue;
        if (materialId > 0UL && currentMaterialId != materialId) continue;
        if (!first) json += ',';
        json += line;
        first = false;
        ++count;
    }

    file.close();
    return true;
}
}
