#ifndef CM_MATERIAL_REQUEST_UNIT_ADAPTER_H
#define CM_MATERIAL_REQUEST_UNIT_ADAPTER_H

#include <Arduino.h>
#include <stdint.h>

#include "CM_MaterialLedger.h"

namespace CM
{
struct MaterialRequestUnitConversion
{
    MaterialUnit ledgerUnit;
    uint32_t ledgerQuantityMilli;
    uint64_t requestUnitCostMinor;
    uint64_t costAmountMinor;

    MaterialRequestUnitConversion()
        : ledgerUnit(MaterialUnit::Piece), ledgerQuantityMilli(0UL),
          requestUnitCostMinor(0ULL), costAmountMinor(0ULL)
    {
    }
};

class MaterialRequestUnitAdapter
{
public:
    static bool convert(const String& requestUnit,
                        uint32_t requestQuantityMilli,
                        MaterialUnit ledgerUnit,
                        uint32_t ledgerPricePerUnitMinor,
                        MaterialRequestUnitConversion& conversion);

    static bool matchesLedgerUnit(const String& requestUnit,
                                  MaterialUnit ledgerUnit);

    static const char* requestUnitForLedger(MaterialUnit ledgerUnit);

private:
    static bool scaleFor(const String& requestUnit,
                         MaterialUnit& ledgerUnit,
                         uint32_t& scale);
};
}

#endif // CM_MATERIAL_REQUEST_UNIT_ADAPTER_H
