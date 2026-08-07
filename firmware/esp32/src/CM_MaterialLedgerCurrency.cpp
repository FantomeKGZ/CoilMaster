#include "CM_MaterialLedger.h"

namespace CM
{
bool MaterialLedger::loadActiveMaterialCurrency(uint32_t materialId,
                                                String& currency,
                                                bool& found) const
{
    currency = String();
    found = false;
    if (!ready() || materialId == 0UL || !m_storage.exists(MaterialsPath))
        return false;

    File file = m_storage.open(MaterialsPath, FILE_READ);
    if (!file) return false;

    uint32_t previousId = 0UL;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;

        uint32_t currentId = 0UL;
        uint32_t stock = 0UL;
        uint32_t price = 0UL;
        String status;
        String storedCurrency;
        if (!line.startsWith("{") || !line.endsWith("}") ||
            !findUnsigned(line, "material_id", currentId) || currentId == 0UL ||
            currentId <= previousId ||
            !findUnsigned(line, "stock_quantity_milli", stock) ||
            !findUnsigned(line, "price_per_unit_minor", price) || price == 0UL ||
            !findString(line, "currency", storedCurrency) || storedCurrency.length() != 3U ||
            !findString(line, "status", status))
        {
            file.close();
            currency = String();
            found = false;
            return false;
        }
        previousId = currentId;

        if (line.indexOf(F("\"comment\":")) >= 0)
        {
            String comment;
            if (!findString(line, "comment", comment))
            {
                file.close();
                currency = String();
                found = false;
                return false;
            }
        }

        if (currentId != materialId) continue;
        if (found || status != "ACTIVE")
        {
            file.close();
            currency = String();
            found = false;
            return false;
        }
        found = true;
        currency = storedCurrency;
    }

    file.close();
    return true;
}

bool MaterialLedger::loadActiveMaterialCurrency(uint32_t materialId,
                                                String& currency) const
{
    bool found = false;
    return loadActiveMaterialCurrency(materialId, currency, found) && found;
}
}
