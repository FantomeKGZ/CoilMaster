#include "CM_MaterialLedger.h"
#include "CM_FlatJsonObjectValidator.h"

namespace CM
{
namespace
{
bool parseMaterialUnit(const String& value, MaterialUnit& unit)
{
    if (value == "PIECE") unit = MaterialUnit::Piece;
    else if (value == "GRAM") unit = MaterialUnit::Gram;
    else if (value == "MILLILITRE") unit = MaterialUnit::Millilitre;
    else if (value == "METRE") unit = MaterialUnit::Metre;
    else if (value == "SQUARE_METRE") unit = MaterialUnit::SquareMetre;
    else return false;
    return true;
}
}

bool MaterialLedger::loadActiveMaterialState(uint32_t materialId,
                                             MaterialItemState& state,
                                             bool& found) const
{
    state = MaterialItemState();
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
        String unitTextValue;
        String status;
        String storedCurrency;
        MaterialUnit parsedUnit = MaterialUnit::Piece;
        if (!FlatJsonObjectValidator::valid(line) ||
            !findUnsigned(line, "material_id", currentId) || currentId == 0UL ||
            currentId <= previousId ||
            !findString(line, "unit", unitTextValue) ||
            !parseMaterialUnit(unitTextValue, parsedUnit) ||
            !findUnsigned(line, "stock_quantity_milli", stock) ||
            !findUnsigned(line, "price_per_unit_minor", price) || price == 0UL ||
            !findString(line, "currency", storedCurrency) || storedCurrency.length() != 3U ||
            !findString(line, "status", status))
        {
            file.close();
            state = MaterialItemState();
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
                state = MaterialItemState();
                found = false;
                return false;
            }
        }

        const bool hasWireType = line.indexOf(F("\"wire_type\":")) >= 0;
        const bool hasDiameter = line.indexOf(F("\"diameter_hundredths_mm\":")) >= 0;
        String wireType;
        uint32_t diameter = 0UL;
        if (hasWireType != hasDiameter ||
            (hasWireType &&
             (!findString(line, "wire_type", wireType) ||
              (wireType != "CU" && wireType != "AL") ||
              !findUnsigned(line, "diameter_hundredths_mm", diameter) ||
              diameter == 0UL || diameter > 65535UL ||
              parsedUnit != MaterialUnit::Gram)))
        {
            file.close();
            state = MaterialItemState();
            found = false;
            return false;
        }

        if (currentId != materialId) continue;
        if (found || status != "ACTIVE")
        {
            file.close();
            state = MaterialItemState();
            found = false;
            return false;
        }

        found = true;
        state.materialId = currentId;
        state.unit = parsedUnit;
        state.stockQuantityMilli = stock;
        state.pricePerUnitMinor = price;
        state.currency = storedCurrency;
        state.hasWireMetadata = hasWireType;
        if (hasWireType)
        {
            state.wireType = wireType;
            state.diameterHundredthsMm = static_cast<uint16_t>(diameter);
        }
    }

    file.close();
    return true;
}

bool MaterialLedger::loadActiveMaterialCurrency(uint32_t materialId,
                                                String& currency,
                                                bool& found) const
{
    currency = String();
    found = false;
    MaterialItemState state;
    if (!loadActiveMaterialState(materialId, state, found)) return false;
    if (found) currency = state.currency;
    return true;
}
}
