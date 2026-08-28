#ifndef CM_MATERIAL_USAGE_IDEMPOTENCY_H
#define CM_MATERIAL_USAGE_IDEMPOTENCY_H

#include <Arduino.h>
#include <FS.h>

namespace CM
{
struct MaterialUsageReplay
{
    bool found;
    bool payloadMatches;
    uint32_t usageId;
    uint32_t repairId;
    uint32_t materialId;
    uint32_t quantityMilli;
    uint32_t unitPriceMinor;
    uint64_t lineCostMinor;
    String currency;

    MaterialUsageReplay()
        : found(false), payloadMatches(false), usageId(0UL), repairId(0UL),
          materialId(0UL), quantityMilli(0UL), unitPriceMinor(0UL),
          lineCostMinor(0ULL), currency("KGS") {}
};

class MaterialUsageIdempotency
{
public:
    static constexpr uint8_t MinOperationIdLength = 16U;
    static constexpr uint8_t MaxOperationIdLength = 64U;

    static bool validOperationId(const String& operationId);
    static String taggedComment(const String& operationId,
                                const String& operatorComment);
    static bool lookup(fs::FS& storage,
                       const String& operationId,
                       uint32_t repairId,
                       uint32_t materialId,
                       uint32_t quantityMilli,
                       MaterialUsageReplay& replay);

private:
    static constexpr const char* UsagePath = "/data/materials/usage.ndjson";
};
}

#endif // CM_MATERIAL_USAGE_IDEMPOTENCY_H
