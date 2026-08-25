#ifndef CM_MATERIAL_REQUEST_WAREHOUSE_PENDING_STORE_H
#define CM_MATERIAL_REQUEST_WAREHOUSE_PENDING_STORE_H

#include <Arduino.h>
#include <FS.h>
#include <stdint.h>

namespace CM
{
struct MaterialRequestWarehousePending
{
    String transactionRef;
    uint32_t materialRequestId;
    uint32_t repairId;
    uint32_t warehouseItemId;
    String movementKind;
    String sourceKind;
    String correctionDirection;
    uint32_t quantityMilliUnits;
    String unit;
    uint64_t unitCostMinor;
    uint64_t costAmountMinor;
    String currency;
    String createdAt;
    String comment;
    uint32_t sourceSessionId;
    uint32_t sourceRunId;
    String materialClass;
    uint16_t wireDiameterHundredthsMm;

    MaterialRequestWarehousePending()
        : materialRequestId(0UL), repairId(0UL), warehouseItemId(0UL),
          quantityMilliUnits(0UL), unitCostMinor(0ULL), costAmountMinor(0ULL),
          sourceSessionId(0UL), sourceRunId(0UL), wireDiameterHundredthsMm(0U)
    {
    }

    bool valid() const;
};

class MaterialRequestWarehousePendingStore
{
public:
    static constexpr const char* Path =
        "/data/workshop/material-request-warehouse.pending.json";
    static constexpr const char* TempPath =
        "/data/workshop/material-request-warehouse.pending.tmp";

    explicit MaterialRequestWarehousePendingStore(fs::FS& storage);

    bool begin();
    bool ready() const;
    bool hasPending() const;
    bool save(const MaterialRequestWarehousePending& pending);
    bool load(MaterialRequestWarehousePending& pending, bool& found) const;
    bool clear();

private:
    bool ensureDirectory();
    bool recoverTemp();
    bool loadPath(const char* path, MaterialRequestWarehousePending& pending) const;

    static bool findUnsigned(const String& line, const char* key, uint32_t& value);
    static bool findUnsigned64(const String& line, const char* key, uint64_t& value);
    static bool findString(const String& line, const char* key, String& value);
    static String jsonEscape(const String& value);
    static void appendUint64(String& target, uint64_t value);

    fs::FS& m_storage;
    bool m_ready;
};
}

#endif // CM_MATERIAL_REQUEST_WAREHOUSE_PENDING_STORE_H
