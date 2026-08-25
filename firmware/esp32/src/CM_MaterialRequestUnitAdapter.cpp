#include "CM_MaterialRequestUnitAdapter.h"

namespace CM
{
bool MaterialRequestUnitAdapter::convert(
    const String& requestUnit,
    uint32_t requestQuantityMilli,
    MaterialUnit ledgerUnit,
    uint32_t ledgerPricePerUnitMinor,
    MaterialRequestUnitConversion& conversion)
{
    conversion = MaterialRequestUnitConversion();
    if (requestQuantityMilli == 0UL || ledgerPricePerUnitMinor == 0UL)
        return false;

    MaterialUnit expectedUnit = MaterialUnit::Piece;
    uint32_t scale = 0UL;
    if (!scaleFor(requestUnit, expectedUnit, scale) || expectedUnit != ledgerUnit ||
        scale == 0UL || requestQuantityMilli > 0xFFFFFFFFUL / scale)
    {
        return false;
    }

    const uint64_t requestUnitCostMinor =
        static_cast<uint64_t>(ledgerPricePerUnitMinor) *
        static_cast<uint64_t>(scale);
    if (requestUnitCostMinor == 0ULL)
        return false;

    if (requestUnitCostMinor >
        (0xFFFFFFFFFFFFFFFFULL - 500ULL) /
            static_cast<uint64_t>(requestQuantityMilli))
    {
        return false;
    }

    const uint64_t costAmountMinor =
        (static_cast<uint64_t>(requestQuantityMilli) * requestUnitCostMinor +
         500ULL) /
        1000ULL;

    conversion.ledgerUnit = ledgerUnit;
    conversion.ledgerQuantityMilli = requestQuantityMilli * scale;
    conversion.requestUnitCostMinor = requestUnitCostMinor;
    conversion.costAmountMinor = costAmountMinor;
    return true;
}

bool MaterialRequestUnitAdapter::matchesLedgerUnit(const String& requestUnit,
                                                   MaterialUnit ledgerUnit)
{
    MaterialUnit expectedUnit = MaterialUnit::Piece;
    uint32_t scale = 0UL;
    return scaleFor(requestUnit, expectedUnit, scale) && scale > 0UL &&
           expectedUnit == ledgerUnit;
}

const char* MaterialRequestUnitAdapter::requestUnitForLedger(MaterialUnit ledgerUnit)
{
    switch (ledgerUnit)
    {
        case MaterialUnit::Gram: return "KG";
        case MaterialUnit::Millilitre: return "L";
        case MaterialUnit::Metre: return "M";
        case MaterialUnit::SquareMetre: return "M2";
        case MaterialUnit::Piece:
        default: return "PCS";
    }
}

bool MaterialRequestUnitAdapter::scaleFor(const String& requestUnit,
                                          MaterialUnit& ledgerUnit,
                                          uint32_t& scale)
{
    scale = 0UL;
    if (requestUnit == "KG")
    {
        ledgerUnit = MaterialUnit::Gram;
        scale = 1000UL;
        return true;
    }
    if (requestUnit == "L")
    {
        ledgerUnit = MaterialUnit::Millilitre;
        scale = 1000UL;
        return true;
    }
    if (requestUnit == "PCS")
    {
        ledgerUnit = MaterialUnit::Piece;
        scale = 1UL;
        return true;
    }
    if (requestUnit == "M")
    {
        ledgerUnit = MaterialUnit::Metre;
        scale = 1UL;
        return true;
    }
    if (requestUnit == "M2")
    {
        ledgerUnit = MaterialUnit::SquareMetre;
        scale = 1UL;
        return true;
    }
    return false;
}
}
