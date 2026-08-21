#pragma once

#include <Arduino.h>
#include <FS.h>
#include <stdint.h>

namespace CM
{
struct WarehouseMovementRepairTotals
{
    uint64_t wireCostMinor;
    uint64_t copperWireCostMinor;
    uint64_t aluminiumWireCostMinor;
    uint64_t unknownWireCostMinor;
    uint32_t copperWireGrams;
    uint32_t aluminiumWireGrams;
    uint32_t unknownWireGrams;
    uint16_t wireLineCount;
    uint16_t copperWireLineCount;
    uint16_t aluminiumWireLineCount;
    uint16_t unknownWireLineCount;
    String currency;
    bool currencySet;

    WarehouseMovementRepairTotals()
        : wireCostMinor(0ULL),
          copperWireCostMinor(0ULL),
          aluminiumWireCostMinor(0ULL),
          unknownWireCostMinor(0ULL),
          copperWireGrams(0UL),
          aluminiumWireGrams(0UL),
          unknownWireGrams(0UL),
          wireLineCount(0U),
          copperWireLineCount(0U),
          aluminiumWireLineCount(0U),
          unknownWireLineCount(0U),
          currencySet(false)
    {
    }
};

class WarehouseMovementIntegrityAudit
{
public:
    static bool check(fs::FS& storage);
    static bool check(fs::FS& storage, uint32_t& validatedRecordCount);
    static bool checkRepair(fs::FS& storage,
                            uint32_t repairId,
                            WarehouseMovementRepairTotals& totals);
};
}
