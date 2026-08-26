#ifndef CM_RUN_WIRE_ISSUE_COORDINATOR_H
#define CM_RUN_WIRE_ISSUE_COORDINATOR_H

#include <Arduino.h>
#include <FS.h>

#include "CM_MaterialLedger.h"
#include "CM_MaterialRequestMovementStore.h"
#include "CM_MaterialRequestStatusStore.h"
#include "CM_MaterialRequestStore.h"
#include "CM_RunWireIssuePendingStore.h"
#include "CM_SpoolMaterialBridgeStore.h"
#include "CM_WarehouseStore.h"

namespace CM
{
struct RunWireIssueResult
{
    uint32_t materialRequestMovementId;
    String transactionRef;
    uint32_t remainingLedgerQuantityMilli;
    uint32_t spoolWeightAfterGrams;
    uint64_t unitCostMinor;
    uint64_t costAmountMinor;
    String currency;

    RunWireIssueResult()
        : materialRequestMovementId(0UL), remainingLedgerQuantityMilli(0UL),
          spoolWeightAfterGrams(0UL), unitCostMinor(0ULL), costAmountMinor(0ULL)
    {
    }
};

class RunWireIssueCoordinator
{
public:
    RunWireIssueCoordinator(fs::FS& storage,
                            MaterialLedger& ledger,
                            MaterialRequestStore& requests,
                            MaterialRequestMovementStore& movements,
                            MaterialRequestStatusStore& statuses,
                            RunWireIssuePendingStore& pending,
                            SpoolMaterialBridgeStore& bridges,
                            WarehouseStore& warehouse);

    bool begin();
    bool ready() const;

    bool execute(const NewMaterialRequestMovement& requestedMovement,
                 uint32_t spoolId,
                 RunWireIssueResult& result);
    bool recover();

private:
    static constexpr const char* UsagePath = "/data/materials/usage.ndjson";

    bool buildPending(const NewMaterialRequestMovement& requestedMovement,
                      uint32_t spoolId,
                      RunWireIssuePending& pending) const;
    bool appendMaterialRequestMovement(const RunWireIssuePending& pending,
                                       uint32_t& movementId);
    bool applyLedger(const RunWireIssuePending& pending,
                     uint32_t& remainingLedgerQuantityMilli);
    bool executePhysicalPhases(const RunWireIssuePending& pending);

    bool movementEvidenceExists(const RunWireIssuePending& pending,
                                bool& found,
                                uint32_t& movementId) const;
    bool ledgerEvidenceExists(const RunWireIssuePending& pending,
                              bool& found) const;
    bool warehouseEvidenceExists(const RunWireIssuePending& pending,
                                 bool& found) const;
    bool spoolStateMatches(const RunWireIssuePending& pending,
                           bool& atBefore,
                           bool& atAfter) const;
    bool requestMatchesRepair(uint32_t materialRequestId,
                              uint32_t repairId,
                              bool& found) const;
    bool requestAllowsWarehouseMutation(uint32_t materialRequestId) const;

    static String taggedComment(const String& transactionRef,
                                const String& comment);
    static String makeTransactionRef(uint32_t materialRequestId,
                                     uint32_t sourceSessionId,
                                     uint32_t sourceRunId);
    static bool prepareNdjson(File& file);
    static bool findUnsigned(const String& line, const char* key, uint32_t& value);
    static bool findUnsigned64(const String& line, const char* key, uint64_t& value);
    static bool findString(const String& line, const char* key, String& value);

    fs::FS& m_storage;
    MaterialLedger& m_ledger;
    MaterialRequestStore& m_requests;
    MaterialRequestMovementStore& m_movements;
    MaterialRequestStatusStore& m_statuses;
    RunWireIssuePendingStore& m_pending;
    SpoolMaterialBridgeStore& m_bridges;
    WarehouseStore& m_warehouse;
    bool m_ready;
};
}

#endif // CM_RUN_WIRE_ISSUE_COORDINATOR_H
