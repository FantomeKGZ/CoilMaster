#include "CM_WarehouseStore.h"

#include "CM_JobSpoolSelectionStore.h"
#include "CM_RepairLifecycle.h"
#include "CM_WindingSessionCompletionAudit.h"

namespace CM
{
namespace
{
bool validateManagedRunWireOperation(WarehouseStore& warehouse,
                                     fs::FS& storage,
                                     const KgFirstWriteOff& operation,
                                     ActiveWireSpoolIdentity& identity)
{
    if (!warehouse.ready() || operation.spoolId == 0UL ||
        operation.repairId == 0UL || operation.sourceSessionId == 0UL ||
        operation.sourceRunId == 0UL || operation.consumedGrams == 0UL ||
        operation.diameterHundredthsMm == 0U ||
        (operation.wireType != "CU" && operation.wireType != "AL") ||
        operation.timestamp.length() < 10U || operation.timestamp.length() > 32U)
    {
        return false;
    }

    bool repairFound = false;
    if (!warehouse.repairExists(operation.repairId, repairFound) || !repairFound)
        return false;

    bool repairOpen = false;
    if (!RepairLifecycle::isOpen(storage, operation.repairId, repairOpen) || !repairOpen)
        return false;

    JobSpoolSelection selection;
    bool selectionFound = false;
    if (!JobSpoolSelectionStore::loadReadOnly(storage,
                                              operation.sourceSessionId,
                                              selection,
                                              selectionFound) ||
        !selectionFound || selection.repairId != operation.repairId ||
        selection.spoolId != operation.spoolId ||
        selection.diameterHundredthsMm != operation.diameterHundredthsMm ||
        selection.wireType != operation.wireType)
    {
        return false;
    }

    if (WindingSessionCompletionAudit::check(storage,
                                             operation.sourceSessionId,
                                             operation.sourceRunId) !=
        WindingSessionCompletionCheck::Completed)
    {
        return false;
    }

    bool alreadyConfirmed = false;
    if (!warehouse.confirmedWriteOffForSourceRun(operation.sourceSessionId,
                                                  operation.sourceRunId,
                                                  alreadyConfirmed) ||
        alreadyConfirmed)
    {
        return false;
    }

    bool found = false;
    if (!warehouse.loadActiveSpoolIdentity(operation.spoolId, identity, found) ||
        !found || !identity.isValid() ||
        identity.diameterHundredthsMm != operation.diameterHundredthsMm ||
        identity.wireType != operation.wireType ||
        operation.consumedGrams >= identity.currentWeightGrams)
    {
        return false;
    }

    return true;
}
}

bool WarehouseStore::prepareManagedRunWireWriteOff(const KgFirstWriteOff& operation,
                                                    uint32_t& movementId)
{
    movementId = 0UL;
    ActiveWireSpoolIdentity identity;
    if (!validateManagedRunWireOperation(*this, m_storage, operation, identity))
        return false;

    WarehousePrice price;
    bool priceConfigured = false;
    if (!loadWarehousePrice(price, priceConfigured) || !priceConfigured)
        return false;

    if (!nextMovementId(movementId)) return false;

    const uint32_t weightBefore = identity.currentWeightGrams;
    const uint32_t weightAfter = weightBefore - operation.consumedGrams;
    if (weightAfter == 0UL) return false;

    return appendKgFirstWriteOffRecord(movementId,
                                       operation,
                                       weightBefore,
                                       weightAfter,
                                       price,
                                       "PENDING");
}

bool WarehouseStore::applyManagedRunWireSpoolWeight(
    uint32_t spoolId,
    uint32_t weightBeforeGrams,
    uint32_t weightAfterGrams,
    uint16_t diameterHundredthsMm,
    const String& wireType)
{
    if (!ready() || spoolId == 0UL || weightBeforeGrams == 0UL ||
        weightAfterGrams == 0UL || weightAfterGrams >= weightBeforeGrams ||
        diameterHundredthsMm == 0U || (wireType != "CU" && wireType != "AL"))
    {
        return false;
    }

    ActiveWireSpoolIdentity identity;
    bool found = false;
    if (!loadActiveSpoolIdentity(spoolId, identity, found) || !found ||
        !identity.isValid() || identity.diameterHundredthsMm != diameterHundredthsMm ||
        identity.wireType != wireType)
    {
        return false;
    }

    // Idempotent recovery: the authoritative transaction may be replayed after
    // the spool mutation was already made durable but before final evidence.
    if (identity.currentWeightGrams == weightAfterGrams) return true;
    if (identity.currentWeightGrams != weightBeforeGrams) return false;

    uint16_t resolvedDiameter = 0U;
    String resolvedWireType;
    return rewriteSpoolWeight(spoolId,
                              weightBeforeGrams,
                              weightAfterGrams,
                              resolvedDiameter,
                              resolvedWireType) &&
           resolvedDiameter == diameterHundredthsMm && resolvedWireType == wireType;
}

bool WarehouseStore::confirmManagedRunWireWriteOff(
    uint32_t movementId,
    const KgFirstWriteOff& operation,
    uint32_t weightBeforeGrams,
    uint32_t weightAfterGrams)
{
    if (!ready() || movementId == 0UL || operation.spoolId == 0UL ||
        weightBeforeGrams == 0UL || weightAfterGrams == 0UL ||
        weightAfterGrams >= weightBeforeGrams ||
        operation.consumedGrams != weightBeforeGrams - weightAfterGrams)
    {
        return false;
    }

    ActiveWireSpoolIdentity identity;
    bool found = false;
    if (!loadActiveSpoolIdentity(operation.spoolId, identity, found) || !found ||
        !identity.isValid() || identity.currentWeightGrams != weightAfterGrams ||
        identity.diameterHundredthsMm != operation.diameterHundredthsMm ||
        identity.wireType != operation.wireType)
    {
        return false;
    }

    WarehousePrice price;
    bool priceConfigured = false;
    if (!loadWarehousePrice(price, priceConfigured) || !priceConfigured)
        return false;

    return appendKgFirstWriteOffRecord(movementId,
                                       operation,
                                       weightBeforeGrams,
                                       weightAfterGrams,
                                       price,
                                       "CONFIRMED");
}
}
