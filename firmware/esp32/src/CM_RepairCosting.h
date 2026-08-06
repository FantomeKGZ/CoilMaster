#ifndef CM_REPAIR_COSTING_H
#define CM_REPAIR_COSTING_H

#include <Arduino.h>
#include <FS.h>

namespace CM
{
struct RepairCostSummary
{
    uint32_t repairId;
    uint64_t wireCostMinor;
    uint64_t copperWireCostMinor;
    uint64_t aluminiumWireCostMinor;
    uint64_t unknownWireCostMinor;
    uint64_t materialCostMinor;
    uint64_t labourCostMinor;
    uint64_t totalCostMinor;
    uint64_t clientPriceMinor;
    uint64_t marginMinor;
    uint32_t copperWireGrams;
    uint32_t aluminiumWireGrams;
    uint32_t unknownWireGrams;
    uint16_t wireLineCount;
    uint16_t copperWireLineCount;
    uint16_t aluminiumWireLineCount;
    uint16_t unknownWireLineCount;
    uint16_t materialLineCount;
    String currency;

    RepairCostSummary()
        : repairId(0UL), wireCostMinor(0ULL), copperWireCostMinor(0ULL),
          aluminiumWireCostMinor(0ULL), unknownWireCostMinor(0ULL),
          materialCostMinor(0ULL), labourCostMinor(0ULL), totalCostMinor(0ULL),
          clientPriceMinor(0ULL), marginMinor(0ULL), copperWireGrams(0UL),
          aluminiumWireGrams(0UL), unknownWireGrams(0UL), wireLineCount(0U),
          copperWireLineCount(0U), aluminiumWireLineCount(0U),
          unknownWireLineCount(0U), materialLineCount(0U), currency("KGS") {}
};

class RepairCosting
{
public:
    explicit RepairCosting(fs::FS& storage);

    bool begin();
    bool ready() const;
    bool load(uint32_t repairId, RepairCostSummary& summary) const;
    bool savePricing(uint32_t repairId,
                     uint64_t labourCostMinor,
                     uint64_t clientPriceMinor,
                     const String& currency,
                     const String& timestamp);

private:
    static constexpr const char* WireMovementsPath = "/data/warehouse/movements.ndjson";
    static constexpr const char* MaterialUsagePath = "/data/materials/usage.ndjson";
    static constexpr const char* PricingPath = "/data/repairs/pricing.ndjson";

    bool ensureDirectories();
    static bool findUnsigned(const String& line, const char* key, uint32_t& value);
    static bool findUnsigned64(const String& line, const char* key, uint64_t& value);
    static bool findString(const String& line, const char* key, String& value);
    static String jsonEscape(const String& value);

    fs::FS& m_storage;
    bool m_ready;
};
}

#endif
