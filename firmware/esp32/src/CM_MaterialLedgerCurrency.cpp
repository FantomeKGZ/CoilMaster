#include "CM_MaterialLedger.h"

namespace CM
{
bool MaterialLedger::loadActiveMaterialCurrency(uint32_t materialId,
                                                String& currency) const
{
    currency = "";
    if (!m_ready || materialId == 0UL || !m_storage.exists(MaterialsPath))
        return false;

    File file = m_storage.open(MaterialsPath, FILE_READ);
    if (!file) return false;

    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        uint32_t currentId = 0UL;
        String status;
        String storedCurrency;
        if (!findUnsigned(line, "material_id", currentId) || currentId != materialId)
            continue;
        if (findString(line, "status", status) && status != "ACTIVE")
            continue;
        if (!findString(line, "currency", storedCurrency))
            continue;

        storedCurrency.trim();
        storedCurrency.toUpperCase();
        currency = storedCurrency;
        file.close();
        return true;
    }

    file.close();
    return false;
}
}
