#ifndef CM_MATERIAL_REQUEST_MOVEMENT_STORE_H
#define CM_MATERIAL_REQUEST_MOVEMENT_STORE_H

#include <Arduino.h>
#include <FS.h>

namespace CM
{
struct NewMaterialRequestMovement
{
    uint32_t materialRequestId;
    uint32_t repairId;
    uint32_t warehouseItemId;
    String transactionRef;      // durable idempotency/provenance token
    String movementKind;        // ISSUE | RETURN | CORRECTION
    String sourceKind;          // MANUAL_MATERIAL | RUN_WIRE
    String correctionDirection; // ADD | REMOVE only for CORRECTION
    uint32_t quantityMilliUnits;
    String unit;                // KG | L | PCS | M | M2
    uint64_t unitCostMinor;
    uint64_t costAmountMinor;
    String currency;
    String createdAt;
    String comment;

    // Required together only for newly persisted RUN_WIRE movements.
    // Historic RUN_WIRE records may not have spool_id; read/integrity code must
    // remain backward-compatible by resolving their immutable session selection.
    uint32_t sourceSessionId;
    uint32_t sourceRunId;
    uint32_t spoolId;
    String materialClass;    // CU | AL for RUN_WIRE
    uint16_t wireDiameterHundredthsMm;

    NewMaterialRequestMovement()
        : materialRequestId(0UL), repairId(0UL), warehouseItemId(0UL),
          quantityMilliUnits(0UL), unitCostMinor(0ULL), costAmountMinor(0ULL),
          sourceSessionId(0UL), sourceRunId(0UL), spoolId(0UL),
          wireDiameterHundredthsMm(0U)
    {
    }
};

class MaterialRequestMovementStore
{
public:
    static constexpr const char* Path =
        "/data/workshop/material-request-movements.ndjson";
    static constexpr uint8_t MaxPageSize = 32U;

    explicit MaterialRequestMovementStore(fs::FS& storage);

    bool begin();
    bool ready() const;
    bool append(const NewMaterialRequestMovement& movement, uint32_t& movementId);
    bool appendRequestPageJson(String& json,
                               uint32_t materialRequestId,
                               uint32_t cursor,
                               uint8_t limit,
                               uint16_t& count,
                               uint32_t& nextCursor,
                               bool& hasMore) const;

private:
    bool ensureDirectory();
    bool nextMovementId(uint32_t& movementId) const;
    static bool validMovement(const NewMaterialRequestMovement& movement);
    static bool validUnit(const String& unit);
    static bool validCurrency(const String& currency);
    static bool findUnsigned(const String& line, const char* key, uint32_t& value);
    static String jsonEscape(const String& value);

    fs::FS& m_storage;
    bool m_ready;
};
}

#endif // CM_MATERIAL_REQUEST_MOVEMENT_STORE_H
