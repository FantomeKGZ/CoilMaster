#ifndef CM_MATERIAL_REQUEST_WAREHOUSE_COORDINATOR_H
#define CM_MATERIAL_REQUEST_WAREHOUSE_COORDINATOR_H

#include <Arduino.h>
#include <FS.h>

#include "CM_MaterialLedger.h"
#include "CM_MaterialRequestMovementStore.h"
#include "CM_MaterialRequestStatusStore.h"
#include "CM_MaterialRequestStore.h"
#include "CM_MaterialRequestWarehousePendingStore.h"

namespace CM
{
struct MaterialRequestWarehouseResult
{
    uint32_t movementId;
    String transactionRef;
    uint32_t remainingLedgerQuantityMilli;
    uint64_t unitCostMinor;
    uint64_t costAmountMinor;
    String currency;

    MaterialRequestWarehouseResult()
        : movementId(0UL), remainingLedgerQuantityMilli(0UL),
          unitCostMinor(0ULL), costAmountMinor(0ULL)
    {
    }
};

class MaterialRequestWarehouseCoordinator
{
public:
    MaterialRequestWarehouseCoordinator(fs::FS& storage,
                                        MaterialLedger& ledger,
                                        MaterialRequestStore& requests,
                                        MaterialRequestMovementStore& movements,
                                        MaterialRequestStatusStore& statuses,
                                        MaterialRequestWarehousePendingStore& pending);

    bool begin();
    bool ready() const;

    bool execute(const NewMaterialRequestMovement& requestedMovement,
                 const String& correctionDirection,
                 MaterialRequestWarehouseResult& result);

    bool recover();

private:
    static constexpr const char* UsagePath = "/data/materials/usage.ndjson";
    static constexpr const char* AdjustmentsPath = "/data/materials/adjustments.ndjson";

    bool buildPending(const NewMaterialRequestMovement& requestedMovement,
                      const String& correctionDirection,
                      MaterialRequestWarehousePending& pending) const;
    bool applyLedger(const MaterialRequestWarehousePending& pending,
                     uint32_t& remainingLedgerQuantityMilli);
    bool movementEvidenceExists(const MaterialRequestWarehousePending& pending,
                                bool& found,
                                uint32_t& movementId) const;
    bool ledgerEvidenceExists(const MaterialRequestWarehousePending& pending,
                              bool& found) const;
    bool requestMatchesRepair(uint32_t materialRequestId,
                              uint32_t repairId,
                              bool& found) const;
    bool requestAllowsWarehouseMutation(uint32_t materialRequestId) const;

    static bool isRemoveMutation(const MaterialRequestWarehousePending& pending);
    static String taggedComment(const String& transactionRef,
                                const String& comment);
    static String makeTransactionRef(uint32_t materialRequestId,
                                     uint32_t warehouseItemId);
    static bool prepareNdjson(File& file);
    static bool findUnsigned(const String& line, const char* key, uint32_t& value);
    static bool findUnsigned64(const String& line, const char* key, uint64_t& value);
    static bool findString(const String& line, const char* key, String& value);

    fs::FS& m_storage;
    MaterialLedger& m_ledger;
    MaterialRequestStore& m_requests;
    MaterialRequestMovementStore& m_movements;
    MaterialRequestStatusStore& m_statuses;
    MaterialRequestWarehousePendingStore& m_pending;
    bool m_ready;
};
}

#endif // CM_MATERIAL_REQUEST_WAREHOUSE_COORDINATOR_H
