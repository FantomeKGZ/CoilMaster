#include "CM_WarehouseStore.h"

namespace CM
{
bool WarehouseStore::loadWarehousePrice(WarehousePrice& price,
                                         bool& configured) const
{
    price = WarehousePrice();
    configured = false;
    if (!ready()) return false;
    if (!m_storage.exists(PricePath)) return true;

    File file = m_storage.open(PricePath, FILE_READ);
    if (!file) return false;

    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;

        uint32_t value = 0UL;
        String currency;
        if (!line.startsWith("{") || !line.endsWith("}") ||
            !findUnsigned(line, "price_per_kg_minor", value) || value == 0UL ||
            !findString(line, "currency", currency) || currency.length() != 3U)
        {
            file.close();
            configured = false;
            price = WarehousePrice();
            return false;
        }

        price.pricePerKgMinor = value;
        price.currency = currency;
        configured = true;
    }

    file.close();
    return true;
}
}
