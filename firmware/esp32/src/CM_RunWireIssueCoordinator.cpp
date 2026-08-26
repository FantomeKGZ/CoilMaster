#include "CM_RunWireIssueCoordinator.h"

#include <esp_system.h>

#include "CM_FlatJsonObjectValidator.h"
#include "CM_JobSpoolSelectionStore.h"
#include "CM_MaterialRequestUnitAdapter.h"
#include "CM_RepairLifecycle.h"
#include "CM_WindingSessionCompletionAudit.h"

namespace CM
{
RunWireIssueCoordinator::RunWireIssueCoordinator(
    fs::FS& storage,
    MaterialLedger& ledger,
    MaterialRequestStore& requests,
    MaterialRequestMovementStore& movements,
    MaterialRequestStatusStore& statuses,
    RunWireIssuePendingStore& pending,
    SpoolMaterialBridgeStore& bridges,
    WarehouseStore& warehouse)
    : m_storage(storage), m_ledger(ledger), m_requests(requests),
      m_movements(movements), m_statuses(statuses), m_pending(pending),
      m_bridges(bridges), m_warehouse(warehouse), m_ready(false)
{
}

bool RunWireIssueCoordinator::begin()
{
    m_ready = m_ledger.ready() && m_requests.ready() && m_movements.ready() &&
              m_statuses.ready() && m_pending.ready() && m_bridges.ready() &&
              m_warehouse.ready();
    if (!m_ready) return false;
    if (!recover())
    {
        m_ready = false;
        return false;
    }
    return true;
}

bool RunWireIssueCoordinator::ready() const
{
    return m_ready && m_ledger.ready() && m_requests.ready() &&
           m_movements.ready() && m_statuses.ready() && m_pending.ready() &&
           m_bridges.ready() && m_warehouse.ready();
}

bool RunWireIssueCoordinator::execute(
    const NewMaterialRequestMovement& requestedMovement,
    uint32_t spoolId,
    RunWireIssueResult& result)
{
    result = RunWireIssueResult();
    if (!ready() || m_pending.hasPending()) return false;

    RunWireIssuePending pending;
    if (!buildPending(requestedMovement, spoolId, pending) ||
        !m_pending.save(pending))
    {
        return false;
    }

    uint32_t materialRequestMovementId = 0UL;
    if (!appendMaterialRequestMovement(pending, materialRequestMovementId))
        return false;

    uint32_t remainingLedgerQuantityMilli = 0UL;
    if (!applyLedger(pending, remainingLedgerQuantityMilli)) return false;

    if (!executePhysicalPhases(pending)) return false;

    bool movementFound = false;
    bool ledgerFound = false;
    bool warehouseFound = false;
    bool spoolAtBefore = false;
    bool spoolAtAfter = false;
    uint32_t verifiedMovementId = 0UL;
    if (!movementEvidenceExists(pending, movementFound, verifiedMovementId) ||
        !ledgerEvidenceExists(pending, ledgerFound) ||
        !warehouseEvidenceExists(pending, warehouseFound) ||
        !spoolStateMatches(pending, spoolAtBefore, spoolAtAfter) ||
        !movementFound || !ledgerFound || !warehouseFound || !spoolAtAfter ||
        spoolAtBefore || verifiedMovementId != materialRequestMovementId ||
        !m_pending.clear())
    {
        return false;
    }

    result.materialRequestMovementId = materialRequestMovementId;
    result.transactionRef = pending.transactionRef;
    result.remainingLedgerQuantityMilli = remainingLedgerQuantityMilli;
    result.spoolWeightAfterGrams = pending.spoolWeightAfterGrams;
    result.unitCostMinor = pending.unitCostMinor;
    result.costAmountMinor = pending.costAmountMinor;
    result.currency = pending.currency;
    return true;
}

bool RunWireIssueCoordinator::recover()
{
    if (!m_ledger.ready() || !m_requests.ready() || !m_movements.ready() ||
        !m_statuses.ready() || !m_pending.ready() || !m_bridges.ready() ||
        !m_warehouse.ready())
    {
        return false;
    }

    RunWireIssuePending pending;
    bool pendingFound = false;
    if (!m_pending.load(pending, pendingFound)) return false;
    if (!pendingFound) return true;

    bool movementFound = false;
    bool ledgerFound = false;
    bool warehouseFound = false;
    bool spoolAtBefore = false;
    bool spoolAtAfter = false;
    uint32_t movementId = 0UL;
    if (!movementEvidenceExists(pending, movementFound, movementId) ||
        !ledgerEvidenceExists(pending, ledgerFound) ||
        !warehouseEvidenceExists(pending, warehouseFound) ||
        !spoolStateMatches(pending, spoolAtBefore, spoolAtAfter))
    {
        return false;
    }

    // No durable effect happened after the authoritative pending save.
    if (!movementFound && !ledgerFound && !warehouseFound && spoolAtBefore)
        return m_pending.clear();

    // Ordering is movement -> ledger -> warehouse evidence -> spool mutation.
    // Any evidence that appears ahead of its predecessor is ambiguous/corrupt.
    if (!movementFound && (ledgerFound || warehouseFound || spoolAtAfter))
        return false;
    if (movementFound && !ledgerFound && (warehouseFound || spoolAtAfter))
        return false;

    if (movementFound && !ledgerFound)
    {
        if (!requestAllowsWarehouseMutation(pending.materialRequestId)) return false;
        uint32_t remaining = 0UL;
        if (!applyLedger(pending, remaining) ||
            !ledgerEvidenceExists(pending, ledgerFound) || !ledgerFound)
        {
            return false;
        }
    }

    if (!movementFound || !ledgerFound) return false;

    if (warehouseFound)
    {
        // Confirmed warehouse evidence is valid only together with exact after-state.
        if (!spoolAtAfter || spoolAtBefore) return false;
        return m_pending.clear();
    }

    // WarehouseStore::begin() has already reconciled any subordinate dangling
    // PENDING. Therefore an unconfirmed phase may be retried only from exact before.
    if (!spoolAtBefore || spoolAtAfter) return false;
    if (!requestAllowsWarehouseMutation(pending.materialRequestId)) return false;
    if (!executePhysicalPhases(pending)) return false;

    if (!warehouseEvidenceExists(pending, warehouseFound) || !warehouseFound ||
        !spoolStateMatches(pending, spoolAtBefore, spoolAtAfter) ||
        !spoolAtAfter || spoolAtBefore)
    {
        return false;
    }

    return m_pending.clear();
}

bool RunWireIssueCoordinator::buildPending(
    const NewMaterialRequestMovement& requestedMovement,
    uint32_t spoolId,
    RunWireIssuePending& pending) const
{
    pending = RunWireIssuePending();
    if (requestedMovement.materialRequestId == 0UL ||
        requestedMovement.repairId == 0UL ||
        requestedMovement.warehouseItemId == 0UL || spoolId == 0UL ||
        requestedMovement.movementKind != "ISSUE" ||
        requestedMovement.sourceKind != "RUN_WIRE" ||
        requestedMovement.unit != "KG" ||
        requestedMovement.quantityMilliUnits == 0UL ||
        requestedMovement.sourceSessionId == 0UL ||
        requestedMovement.sourceRunId == 0UL ||
        (requestedMovement.materialClass != "CU" &&
         requestedMovement.materialClass != "AL") ||
        requestedMovement.wireDiameterHundredthsMm == 0U ||
        requestedMovement.createdAt.length() < 10U ||
        requestedMovement.createdAt.length() > 32U)
    {
        return false;
    }

    bool requestFound = false;
    if (!requestMatchesRepair(requestedMovement.materialRequestId,
                              requestedMovement.repairId,
                              requestFound) ||
        !requestFound ||
        !requestAllowsWarehouseMutation(requestedMovement.materialRequestId))
    {
        return false;
    }

    bool repairOpen = false;
    if (!RepairLifecycle::isOpen(m_storage, requestedMovement.repairId, repairOpen) ||
        !repairOpen)
    {
        return false;
    }

    JobSpoolSelection selection;
    bool selectionFound = false;
    if (!JobSpoolSelectionStore::loadReadOnly(m_storage,
                                              requestedMovement.sourceSessionId,
                                              selection,
                                              selectionFound) ||
        !selectionFound || selection.repairId != requestedMovement.repairId ||
        selection.spoolId != spoolId ||
        selection.diameterHundredthsMm != requestedMovement.wireDiameterHundredthsMm ||
        selection.wireType != requestedMovement.materialClass)
    {
        return false;
    }

    if (WindingSessionCompletionAudit::check(m_storage,
                                             requestedMovement.sourceSessionId,
                                             requestedMovement.sourceRunId) !=
        WindingSessionCompletionCheck::Completed)
    {
        return false;
    }

    SpoolMaterialBridge bridge;
    bool bridgeFound = false;
    if (!m_bridges.loadBySpool(spoolId, bridge, bridgeFound) || !bridgeFound ||
        !bridge.valid() || bridge.warehouseItemId != requestedMovement.warehouseItemId ||
        bridge.wireType != requestedMovement.materialClass ||
        bridge.diameterHundredthsMm != requestedMovement.wireDiameterHundredthsMm)
    {
        return false;
    }

    MaterialItemState item;
    bool itemFound = false;
    if (!m_ledger.loadActiveMaterialState(requestedMovement.warehouseItemId,
                                          item,
                                          itemFound) ||
        !itemFound || item.currency != "KGS" || !item.hasWireMetadata ||
        item.unit != MaterialUnit::Gram || item.wireType != bridge.wireType ||
        item.diameterHundredthsMm != bridge.diameterHundredthsMm)
    {
        return false;
    }

    MaterialRequestUnitConversion conversion;
    if (!MaterialRequestUnitAdapter::convert("KG",
                                             requestedMovement.quantityMilliUnits,
                                             item.unit,
                                             item.pricePerUnitMinor,
                                             conversion) ||
        conversion.ledgerQuantityMilli > item.stockQuantityMilli)
    {
        return false;
    }

    // Atomic RUN_WIRE must have one price authority. MaterialLedger stores the
    // wire item's price per gram while the physical warehouse movement/costing
    // stores price per kg. Their exact KG-equivalent values must agree before
    // any high-level pending intent is persisted.
    WarehousePrice warehousePrice;
    bool warehousePriceConfigured = false;
    if (!m_warehouse.loadWarehousePrice(warehousePrice, warehousePriceConfigured) ||
        !warehousePriceConfigured || warehousePrice.currency != item.currency ||
        conversion.requestUnitCostMinor > 0xFFFFFFFFULL ||
        warehousePrice.pricePerKgMinor !=
            static_cast<uint32_t>(conversion.requestUnitCostMinor))
    {
        return false;
    }

    ActiveWireSpoolIdentity spool;
    bool spoolFound = false;
    if (!m_warehouse.loadActiveSpoolIdentity(spoolId, spool, spoolFound) ||
        !spoolFound || !spool.isValid() ||
        spool.diameterHundredthsMm != bridge.diameterHundredthsMm ||
        spool.wireType != bridge.wireType ||
        requestedMovement.quantityMilliUnits >= spool.currentWeightGrams)
    {
        return false;
    }

    bool alreadyConfirmed = false;
    if (!m_warehouse.confirmedWriteOffForSourceRun(requestedMovement.sourceSessionId,
                                                    requestedMovement.sourceRunId,
                                                    alreadyConfirmed) ||
        alreadyConfirmed)
    {
        return false;
    }

    pending.transactionRef = makeTransactionRef(requestedMovement.materialRequestId,
                                                requestedMovement.sourceSessionId,
                                                requestedMovement.sourceRunId);
    pending.materialRequestId = requestedMovement.materialRequestId;
    pending.repairId = requestedMovement.repairId;
    pending.warehouseItemId = requestedMovement.warehouseItemId;
    pending.sourceSessionId = requestedMovement.sourceSessionId;
    pending.sourceRunId = requestedMovement.sourceRunId;
    pending.spoolId = spoolId;
    // For a KG request, one milli-unit is exactly one gram.
    pending.consumedGrams = requestedMovement.quantityMilliUnits;
    pending.spoolWeightBeforeGrams = spool.currentWeightGrams;
    pending.spoolWeightAfterGrams = spool.currentWeightGrams - pending.consumedGrams;
    pending.ledgerQuantityMilli = conversion.ledgerQuantityMilli;
    pending.unitCostMinor = conversion.requestUnitCostMinor;
    pending.costAmountMinor = conversion.costAmountMinor;
    pending.currency = item.currency;
    pending.materialClass = bridge.wireType;
    pending.wireDiameterHundredthsMm = bridge.diameterHundredthsMm;
    pending.createdAt = requestedMovement.createdAt;
    pending.comment = requestedMovement.comment;
    return pending.valid();
}

bool RunWireIssueCoordinator::appendMaterialRequestMovement(
    const RunWireIssuePending& pending,
    uint32_t& movementId)
{
    NewMaterialRequestMovement movement;
    movement.materialRequestId = pending.materialRequestId;
    movement.repairId = pending.repairId;
    movement.warehouseItemId = pending.warehouseItemId;
    movement.transactionRef = pending.transactionRef;
    movement.movementKind = "ISSUE";
    movement.sourceKind = "RUN_WIRE";
    movement.quantityMilliUnits = pending.consumedGrams;
    movement.unit = "KG";
    movement.unitCostMinor = pending.unitCostMinor;
    movement.costAmountMinor = pending.costAmountMinor;
    movement.currency = pending.currency;
    movement.createdAt = pending.createdAt;
    movement.comment = pending.comment;
    movement.sourceSessionId = pending.sourceSessionId;
    movement.sourceRunId = pending.sourceRunId;
    movement.materialClass = pending.materialClass;
    movement.wireDiameterHundredthsMm = pending.wireDiameterHundredthsMm;
    return m_movements.append(movement, movementId);
}

bool RunWireIssueCoordinator::applyLedger(
    const RunWireIssuePending& pending,
    uint32_t& remainingLedgerQuantityMilli)
{
    remainingLedgerQuantityMilli = 0UL;
    MaterialItemState item;
    bool found = false;
    if (!m_ledger.loadActiveMaterialState(pending.warehouseItemId, item, found) ||
        !found || !item.hasWireMetadata || item.unit != MaterialUnit::Gram ||
        item.wireType != pending.materialClass ||
        item.diameterHundredthsMm != pending.wireDiameterHundredthsMm ||
        item.currency != pending.currency)
    {
        return false;
    }

    MaterialRequestUnitConversion conversion;
    if (!MaterialRequestUnitAdapter::convert("KG",
                                             pending.consumedGrams,
                                             item.unit,
                                             item.pricePerUnitMinor,
                                             conversion) ||
        conversion.ledgerQuantityMilli != pending.ledgerQuantityMilli ||
        conversion.requestUnitCostMinor != pending.unitCostMinor ||
        conversion.costAmountMinor != pending.costAmountMinor)
    {
        return false;
    }

    RepairMaterialUsage usage;
    usage.repairId = pending.repairId;
    usage.materialId = pending.warehouseItemId;
    usage.quantityMilli = pending.ledgerQuantityMilli;
    usage.timestamp = pending.createdAt;
    usage.comment = taggedComment(pending.transactionRef, pending.comment);

    RepairMaterialUsageResult result;
    if (!m_ledger.confirmUsage(usage, result) ||
        result.currency != pending.currency ||
        result.lineCostMinor != pending.costAmountMinor)
    {
        return false;
    }
    remainingLedgerQuantityMilli = result.remainingQuantityMilli;
    return true;
}

bool RunWireIssueCoordinator::executePhysicalPhases(
    const RunWireIssuePending& pending)
{
    // The warehouse price may change after pending persistence (including across
    // reboot). Never commit physical/costing evidence with a different price
    // than the immutable Material Request / Ledger transaction already carries.
    WarehousePrice warehousePrice;
    bool warehousePriceConfigured = false;
    if (!m_warehouse.loadWarehousePrice(warehousePrice, warehousePriceConfigured) ||
        !warehousePriceConfigured || warehousePrice.currency != pending.currency ||
        pending.unitCostMinor > 0xFFFFFFFFULL ||
        warehousePrice.pricePerKgMinor != static_cast<uint32_t>(pending.unitCostMinor))
    {
        return false;
    }

    KgFirstWriteOff operation;
    operation.spoolId = pending.spoolId;
    operation.repairId = pending.repairId;
    operation.sourceSessionId = pending.sourceSessionId;
    operation.sourceRunId = pending.sourceRunId;
    operation.diameterHundredthsMm = pending.wireDiameterHundredthsMm;
    operation.consumedGrams = pending.consumedGrams;
    operation.wireType = pending.materialClass;
    operation.timestamp = pending.createdAt;
    operation.comment = taggedComment(pending.transactionRef, pending.comment);

    uint32_t warehouseMovementId = 0UL;
    if (!m_warehouse.prepareManagedRunWireWriteOff(operation, warehouseMovementId))
        return false;

    if (!m_warehouse.applyManagedRunWireSpoolWeight(
            pending.spoolId,
            pending.spoolWeightBeforeGrams,
            pending.spoolWeightAfterGrams,
            pending.wireDiameterHundredthsMm,
            pending.materialClass))
    {
        m_ready = false;
        return false;
    }

    if (!m_warehouse.confirmManagedRunWireWriteOff(
            warehouseMovementId,
            operation,
            pending.spoolWeightBeforeGrams,
            pending.spoolWeightAfterGrams))
    {
        m_ready = false;
        return false;
    }
    return true;
}

bool RunWireIssueCoordinator::movementEvidenceExists(
    const RunWireIssuePending& pending,
    bool& found,
    uint32_t& movementId) const
{
    found = false;
    movementId = 0UL;
    if (!m_storage.exists(MaterialRequestMovementStore::Path)) return true;
    File file = m_storage.open(MaterialRequestMovementStore::Path, FILE_READ);
    if (!prepareNdjson(file))
    {
        if (file) file.close();
        return false;
    }

    uint32_t previousId = 0UL;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        uint32_t id = 0UL, requestId = 0UL, repairId = 0UL, itemId = 0UL;
        uint32_t quantity = 0UL, sessionId = 0UL, runId = 0UL, diameter = 0UL;
        uint64_t unitCost = 0ULL, amount = 0ULL;
        String transactionRef, movementKind, sourceKind, unit, currency, materialClass;
        if (!FlatJsonObjectValidator::valid(line) ||
            !findUnsigned(line, "movement_id", id) || id == 0UL || id <= previousId ||
            !findUnsigned(line, "material_request_id", requestId) ||
            !findUnsigned(line, "repair_id", repairId) ||
            !findUnsigned(line, "warehouse_item_id", itemId) ||
            !findString(line, "transaction_ref", transactionRef) ||
            !findString(line, "movement_kind", movementKind) ||
            !findString(line, "source_kind", sourceKind) ||
            !findUnsigned(line, "quantity_milli_units", quantity) ||
            !findString(line, "unit", unit) ||
            !findUnsigned64(line, "unit_cost_minor", unitCost) ||
            !findUnsigned64(line, "cost_amount_minor", amount) ||
            !findString(line, "currency", currency))
        {
            file.close();
            return false;
        }
        previousId = id;
        if (transactionRef != pending.transactionRef) continue;

        if (found || requestId != pending.materialRequestId ||
            repairId != pending.repairId || itemId != pending.warehouseItemId ||
            movementKind != "ISSUE" || sourceKind != "RUN_WIRE" ||
            quantity != pending.consumedGrams || unit != "KG" ||
            unitCost != pending.unitCostMinor || amount != pending.costAmountMinor ||
            currency != pending.currency ||
            !findUnsigned(line, "source_session_id", sessionId) ||
            !findUnsigned(line, "source_run_id", runId) ||
            !findString(line, "material_class", materialClass) ||
            !findUnsigned(line, "wire_diameter_hundredths_mm", diameter) ||
            sessionId != pending.sourceSessionId || runId != pending.sourceRunId ||
            materialClass != pending.materialClass ||
            diameter != pending.wireDiameterHundredthsMm)
        {
            file.close();
            return false;
        }
        found = true;
        movementId = id;
    }
    file.close();
    return true;
}

bool RunWireIssueCoordinator::ledgerEvidenceExists(
    const RunWireIssuePending& pending,
    bool& found) const
{
    found = false;
    if (!m_storage.exists(UsagePath)) return true;
    File file = m_storage.open(UsagePath, FILE_READ);
    if (!prepareNdjson(file))
    {
        if (file) file.close();
        return false;
    }

    const String marker = String("RWI_TX=") + pending.transactionRef + ';';
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;
        if (!FlatJsonObjectValidator::valid(line))
        {
            file.close();
            return false;
        }
        String comment;
        if (!findString(line, "comment", comment) || comment.indexOf(marker) != 0)
            continue;

        uint32_t materialId = 0UL, repairId = 0UL, quantity = 0UL;
        if (found ||
            !findUnsigned(line, "material_id", materialId) ||
            !findUnsigned(line, "repair_id", repairId) ||
            !findUnsigned(line, "quantity_milli", quantity) ||
            materialId != pending.warehouseItemId || repairId != pending.repairId ||
            quantity != pending.ledgerQuantityMilli)
        {
            file.close();
            return false;
        }
        found = true;
    }
    file.close();
    return true;
}

bool RunWireIssueCoordinator::warehouseEvidenceExists(
    const RunWireIssuePending& pending,
    bool& found) const
{
    return m_warehouse.confirmedWriteOffForSourceRun(pending.sourceSessionId,
                                                      pending.sourceRunId,
                                                      found);
}

bool RunWireIssueCoordinator::spoolStateMatches(
    const RunWireIssuePending& pending,
    bool& atBefore,
    bool& atAfter) const
{
    atBefore = false;
    atAfter = false;
    ActiveWireSpoolIdentity identity;
    bool found = false;
    if (!m_warehouse.loadActiveSpoolIdentity(pending.spoolId, identity, found) ||
        !found || !identity.isValid() ||
        identity.diameterHundredthsMm != pending.wireDiameterHundredthsMm ||
        identity.wireType != pending.materialClass)
    {
        return false;
    }
    atBefore = identity.currentWeightGrams == pending.spoolWeightBeforeGrams;
    atAfter = identity.currentWeightGrams == pending.spoolWeightAfterGrams;
    return atBefore || atAfter;
}

bool RunWireIssueCoordinator::requestMatchesRepair(
    uint32_t materialRequestId,
    uint32_t repairId,
    bool& found) const
{
    found = false;
    String json;
    if (!m_requests.appendByIdJson(json, materialRequestId, found) || !found)
        return found;
    uint32_t storedRepairId = 0UL;
    return FlatJsonObjectValidator::valid(json) &&
           findUnsigned(json, "repair_id", storedRepairId) &&
           storedRepairId == repairId;
}

bool RunWireIssueCoordinator::requestAllowsWarehouseMutation(
    uint32_t materialRequestId) const
{
    MaterialRequestStatusState state;
    bool found = false;
    return m_statuses.resolve(materialRequestId, state, found) && found &&
           (state.status == "DRAFT" || state.status == "ISSUED");
}

String RunWireIssueCoordinator::taggedComment(const String& transactionRef,
                                              const String& comment)
{
    String tagged = String("RWI_TX=") + transactionRef + ';';
    if (comment.length() > 0U)
    {
        tagged += ' ';
        tagged += comment;
    }
    return tagged;
}

String RunWireIssueCoordinator::makeTransactionRef(uint32_t materialRequestId,
                                                   uint32_t sourceSessionId,
                                                   uint32_t sourceRunId)
{
    char buffer[80];
    snprintf(buffer, sizeof(buffer), "RWI-%lu-%lu-%lu-%08lx-%08lx",
             static_cast<unsigned long>(materialRequestId),
             static_cast<unsigned long>(sourceSessionId),
             static_cast<unsigned long>(sourceRunId),
             static_cast<unsigned long>(millis()),
             static_cast<unsigned long>(esp_random()));
    return String(buffer);
}

bool RunWireIssueCoordinator::prepareNdjson(File& file)
{
    if (!file || file.isDirectory()) return false;
    const size_t rawSize = file.size();
    if (rawSize > 0xFFFFFFFFUL) return false;
    if (rawSize == 0U) return file.seek(0U);
    if (!file.seek(static_cast<uint32_t>(rawSize - 1U)) || file.read() != '\n')
        return false;
    return file.seek(0U);
}

bool RunWireIssueCoordinator::findUnsigned(const String& line,
                                           const char* key,
                                           uint32_t& value)
{
    uint64_t parsed = 0ULL;
    if (!findUnsigned64(line, key, parsed) || parsed > 0xFFFFFFFFULL) return false;
    value = static_cast<uint32_t>(parsed);
    return true;
}

bool RunWireIssueCoordinator::findUnsigned64(const String& line,
                                             const char* key,
                                             uint64_t& value)
{
    value = 0ULL;
    const String marker = String('"') + key + F("\":");
    const int start = line.indexOf(marker);
    if (start < 0 || line.indexOf(marker, start + marker.length()) >= 0) return false;
    size_t pos = static_cast<size_t>(start) + marker.length();
    if (pos >= line.length() || !isDigit(line[pos])) return false;
    uint64_t parsed = 0ULL;
    while (pos < line.length() && isDigit(line[pos]))
    {
        const uint8_t digit = static_cast<uint8_t>(line[pos] - '0');
        if (parsed > (UINT64_MAX - digit) / 10ULL) return false;
        parsed = parsed * 10ULL + digit;
        ++pos;
    }
    if (pos >= line.length() || (line[pos] != ',' && line[pos] != '}')) return false;
    value = parsed;
    return true;
}

bool RunWireIssueCoordinator::findString(const String& line,
                                         const char* key,
                                         String& value)
{
    value = String();
    const String marker = String('"') + key + F("\":\"");
    const int start = line.indexOf(marker);
    if (start < 0 || line.indexOf(marker, start + marker.length()) >= 0) return false;
    size_t pos = static_cast<size_t>(start) + marker.length();
    bool escaped = false;
    while (pos < line.length())
    {
        const char ch = line[pos++];
        if (!escaped && ch == '"')
            return pos < line.length() && (line[pos] == ',' || line[pos] == '}');
        if (!escaped && ch == '\\')
        {
            escaped = true;
            continue;
        }
        if (escaped)
        {
            if (ch == '"' || ch == '\\') value += ch;
            else if (ch == 'n') value += '\n';
            else if (ch == 'r') value += '\r';
            else if (ch == 't') value += '\t';
            else return false;
            escaped = false;
            continue;
        }
        if (static_cast<uint8_t>(ch) < 0x20U) return false;
        value += ch;
    }
    return false;
}
}
