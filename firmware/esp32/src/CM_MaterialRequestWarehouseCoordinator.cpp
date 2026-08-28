#include "CM_MaterialRequestWarehouseCoordinator.h"

#include <esp_system.h>

#include "CM_FlatJsonObjectValidator.h"
#include "CM_MaterialRequestUnitAdapter.h"
#include "CM_RepairLifecycle.h"

namespace CM
{
MaterialRequestWarehouseCoordinator::MaterialRequestWarehouseCoordinator(
    fs::FS& storage,
    MaterialLedger& ledger,
    MaterialRequestStore& requests,
    MaterialRequestMovementStore& movements,
    MaterialRequestStatusStore& statuses,
    MaterialRequestWarehousePendingStore& pending)
    : m_storage(storage), m_ledger(ledger), m_requests(requests),
      m_movements(movements), m_statuses(statuses), m_pending(pending),
      m_ready(false)
{
}

bool MaterialRequestWarehouseCoordinator::begin()
{
    m_ready = m_ledger.ready() && m_requests.ready() && m_movements.ready() &&
              m_statuses.ready() && m_pending.ready();
    if (!m_ready) return false;
    if (!recover())
    {
        m_ready = false;
        return false;
    }
    return true;
}

bool MaterialRequestWarehouseCoordinator::ready() const
{
    return m_ready && m_ledger.ready() && m_requests.ready() &&
           m_movements.ready() && m_statuses.ready() && m_pending.ready();
}

bool MaterialRequestWarehouseCoordinator::execute(
    const NewMaterialRequestMovement& requestedMovement,
    const String& correctionDirection,
    MaterialRequestWarehouseResult& result)
{
    result = MaterialRequestWarehouseResult();
    if (!ready() || m_pending.hasPending()) return false;

    MaterialRequestWarehousePending pending;
    if (!buildPending(requestedMovement, correctionDirection, pending) ||
        !m_pending.save(pending))
    {
        return false;
    }

    NewMaterialRequestMovement movement = requestedMovement;
    movement.transactionRef = pending.transactionRef;
    movement.correctionDirection = pending.correctionDirection;
    movement.unitCostMinor = pending.unitCostMinor;
    movement.costAmountMinor = pending.costAmountMinor;
    movement.currency = pending.currency;
    movement.comment = pending.comment;

    uint32_t movementId = 0UL;
    if (!m_movements.append(movement, movementId)) return false;

    uint32_t remaining = 0UL;
    if (!applyLedger(pending, remaining)) return false;

    bool movementFound = false;
    bool ledgerFound = false;
    uint32_t verifiedMovementId = 0UL;
    if (!movementEvidenceExists(pending, movementFound, verifiedMovementId) ||
        !ledgerEvidenceExists(pending, ledgerFound) ||
        !movementFound || !ledgerFound || verifiedMovementId != movementId ||
        !m_pending.clear())
    {
        return false;
    }

    result.movementId = movementId;
    result.transactionRef = pending.transactionRef;
    result.remainingLedgerQuantityMilli = remaining;
    result.unitCostMinor = pending.unitCostMinor;
    result.costAmountMinor = pending.costAmountMinor;
    result.currency = pending.currency;
    return true;
}

bool MaterialRequestWarehouseCoordinator::recover()
{
    if (!m_ledger.ready() || !m_requests.ready() || !m_movements.ready() ||
        !m_statuses.ready() || !m_pending.ready())
    {
        return false;
    }

    MaterialRequestWarehousePending pending;
    bool pendingFound = false;
    if (!m_pending.load(pending, pendingFound)) return false;
    if (!pendingFound) return true;

    bool movementFound = false;
    bool ledgerFound = false;
    uint32_t movementId = 0UL;
    if (!movementEvidenceExists(pending, movementFound, movementId) ||
        !ledgerEvidenceExists(pending, ledgerFound))
    {
        return false;
    }

    if (!movementFound && !ledgerFound)
        return m_pending.clear();

    // Ledger-only is impossible with movement-first ordering and therefore
    // means ambiguous/corrupt evidence. Never guess or rewrite history.
    if (!movementFound && ledgerFound) return false;

    if (movementFound && !ledgerFound)
    {
        if (!requestAllowsWarehouseMutation(pending.materialRequestId)) return false;
        uint32_t remaining = 0UL;
        if (!applyLedger(pending, remaining)) return false;
        if (!ledgerEvidenceExists(pending, ledgerFound) || !ledgerFound) return false;
    }

    return m_pending.clear();
}

bool MaterialRequestWarehouseCoordinator::buildPending(
    const NewMaterialRequestMovement& requestedMovement,
    const String& correctionDirection,
    MaterialRequestWarehousePending& pending) const
{
    pending = MaterialRequestWarehousePending();
    if (requestedMovement.materialRequestId == 0UL ||
        requestedMovement.repairId == 0UL ||
        requestedMovement.warehouseItemId == 0UL ||
        requestedMovement.quantityMilliUnits == 0UL ||
        requestedMovement.createdAt.length() < 10U ||
        requestedMovement.createdAt.length() > 32U)
    {
        return false;
    }

    bool requestFound = false;
    if (!requestMatchesRepair(requestedMovement.materialRequestId,
                              requestedMovement.repairId, requestFound) ||
        !requestFound ||
        !knownRequestAllowsWarehouseMutation(requestedMovement.materialRequestId))
    {
        return false;
    }

    bool repairOpen = false;
    if (!RepairLifecycle::isOpen(m_storage, requestedMovement.repairId, repairOpen) ||
        !repairOpen)
    {
        return false;
    }

    MaterialItemState item;
    bool itemFound = false;
    if (!m_ledger.loadActiveMaterialState(requestedMovement.warehouseItemId,
                                          item, itemFound) ||
        !itemFound || item.currency != "KGS")
    {
        return false;
    }

    MaterialRequestUnitConversion conversion;
    if (!MaterialRequestUnitAdapter::convert(requestedMovement.unit,
                                             requestedMovement.quantityMilliUnits,
                                             item.unit,
                                             item.pricePerUnitMinor,
                                             conversion))
    {
        return false;
    }

    pending.transactionRef = makeTransactionRef(requestedMovement.materialRequestId,
                                                requestedMovement.warehouseItemId);
    pending.materialRequestId = requestedMovement.materialRequestId;
    pending.repairId = requestedMovement.repairId;
    pending.warehouseItemId = requestedMovement.warehouseItemId;
    pending.movementKind = requestedMovement.movementKind;
    pending.sourceKind = requestedMovement.sourceKind;
    pending.correctionDirection = correctionDirection;
    pending.quantityMilliUnits = requestedMovement.quantityMilliUnits;
    pending.unit = requestedMovement.unit;
    pending.unitCostMinor = conversion.requestUnitCostMinor;
    pending.costAmountMinor = conversion.costAmountMinor;
    pending.currency = item.currency;
    pending.createdAt = requestedMovement.createdAt;
    pending.comment = requestedMovement.comment;
    pending.sourceSessionId = requestedMovement.sourceSessionId;
    pending.sourceRunId = requestedMovement.sourceRunId;
    pending.materialClass = requestedMovement.materialClass;
    pending.wireDiameterHundredthsMm = requestedMovement.wireDiameterHundredthsMm;

    if (!pending.valid()) return false;
    if (isRemoveMutation(pending) && item.stockQuantityMilli < conversion.ledgerQuantityMilli)
        return false;
    return true;
}

bool MaterialRequestWarehouseCoordinator::applyLedger(
    const MaterialRequestWarehousePending& pending,
    uint32_t& remainingLedgerQuantityMilli)
{
    remainingLedgerQuantityMilli = 0UL;
    MaterialItemState item;
    bool itemFound = false;
    if (!m_ledger.loadActiveMaterialState(pending.warehouseItemId, item, itemFound) ||
        !itemFound || item.currency != pending.currency)
    {
        return false;
    }

    MaterialRequestUnitConversion conversion;
    if (!MaterialRequestUnitAdapter::convert(pending.unit,
                                             pending.quantityMilliUnits,
                                             item.unit,
                                             item.pricePerUnitMinor,
                                             conversion) ||
        conversion.requestUnitCostMinor != pending.unitCostMinor ||
        conversion.costAmountMinor != pending.costAmountMinor)
    {
        return false;
    }

    const String comment = taggedComment(pending.transactionRef, pending.comment);
    if (isRemoveMutation(pending))
    {
        RepairMaterialUsage usage;
        usage.repairId = pending.repairId;
        usage.materialId = pending.warehouseItemId;
        usage.quantityMilli = conversion.ledgerQuantityMilli;
        usage.timestamp = pending.createdAt;
        usage.comment = comment;
        RepairMaterialUsageResult usageResult;
        if (!m_ledger.confirmUsage(usage, usageResult) ||
            usageResult.currency != pending.currency ||
            usageResult.lineCostMinor != pending.costAmountMinor)
        {
            return false;
        }
        remainingLedgerQuantityMilli = usageResult.remainingQuantityMilli;
        return true;
    }

    MaterialAdjustment adjustment;
    adjustment.materialId = pending.warehouseItemId;
    adjustment.addQuantityMilli = conversion.ledgerQuantityMilli;
    adjustment.newPricePerUnitMinor = 0UL;
    adjustment.currency = pending.currency;
    adjustment.timestamp = pending.createdAt;
    adjustment.comment = comment;
    MaterialAdjustmentResult adjustmentResult;
    if (!m_ledger.adjustMaterial(adjustment, adjustmentResult) ||
        adjustmentResult.currency != pending.currency)
    {
        return false;
    }
    remainingLedgerQuantityMilli = adjustmentResult.stockQuantityMilli;
    return true;
}

bool MaterialRequestWarehouseCoordinator::movementEvidenceExists(
    const MaterialRequestWarehousePending& pending,
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
        String transactionRef, movementKind, sourceKind, unit, currency;
        uint64_t unitCost = 0ULL, amount = 0ULL;
        if (!FlatJsonObjectValidator::valid(line) ||
            !findUnsigned(line, "movement_id", id) || id == 0UL || id <= previousId ||
            !findUnsigned(line, "material_request_id", requestId) ||
            !findUnsigned(line, "repair_id", repairId) ||
            !findUnsigned(line, "warehouse_item_id", itemId) ||
            !findString(line, "transaction_ref", transactionRef) ||
            !findString(line, "movement_kind", movementKind) ||
            !findString(line, "source_kind", sourceKind) ||
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
            movementKind != pending.movementKind || sourceKind != pending.sourceKind ||
            unit != pending.unit || unitCost != pending.unitCostMinor ||
            amount != pending.costAmountMinor || currency != pending.currency)
        {
            file.close();
            return false;
        }
        if (pending.movementKind == "CORRECTION")
        {
            String direction;
            if (!findString(line, "correction_direction", direction) ||
                direction != pending.correctionDirection)
            {
                file.close();
                return false;
            }
        }
        found = true;
        movementId = id;
    }
    file.close();
    return true;
}

bool MaterialRequestWarehouseCoordinator::ledgerEvidenceExists(
    const MaterialRequestWarehousePending& pending,
    bool& found) const
{
    found = false;
    const char* path = isRemoveMutation(pending) ? UsagePath : AdjustmentsPath;
    if (!m_storage.exists(path)) return true;
    File file = m_storage.open(path, FILE_READ);
    if (!prepareNdjson(file))
    {
        if (file) file.close();
        return false;
    }

    const String marker = String("MRW_TX=") + pending.transactionRef + ';';
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

        uint32_t itemId = 0UL;
        if (!findUnsigned(line, "material_id", itemId) ||
            itemId != pending.warehouseItemId || found)
        {
            file.close();
            return false;
        }
        if (isRemoveMutation(pending))
        {
            uint32_t repairId = 0UL;
            if (!findUnsigned(line, "repair_id", repairId) ||
                repairId != pending.repairId)
            {
                file.close();
                return false;
            }
        }
        found = true;
    }
    file.close();
    return true;
}

bool MaterialRequestWarehouseCoordinator::requestMatchesRepair(
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

bool MaterialRequestWarehouseCoordinator::requestAllowsWarehouseMutation(
    uint32_t materialRequestId) const
{
    MaterialRequestStatusState state;
    bool found = false;
    return m_statuses.resolve(materialRequestId, state, found) && found &&
           (state.status == "DRAFT" || state.status == "ISSUED");
}

bool MaterialRequestWarehouseCoordinator::knownRequestAllowsWarehouseMutation(
    uint32_t materialRequestId) const
{
    if (!m_statuses.ready() || materialRequestId == 0UL) return false;

    MaterialRequestStatusState state;
    state.materialRequestId = materialRequestId;
    state.status = "DRAFT";

    if (!m_storage.exists(MaterialRequestStatusStore::Path)) return true;
    File file = m_storage.open(MaterialRequestStatusStore::Path, FILE_READ);
    if (!prepareNdjson(file))
    {
        if (file) file.close();
        return false;
    }

    uint32_t previousTransitionId = 0UL;
    while (file.available())
    {
        const String line = file.readStringUntil('\n');
        if (line.length() == 0U) continue;

        uint32_t transitionId = 0UL;
        uint32_t requestId = 0UL;
        String fromStatus;
        String toStatus;
        String changedAt;
        if (!FlatJsonObjectValidator::valid(line) ||
            !findUnsigned(line, "transition_id", transitionId) || transitionId == 0UL ||
            transitionId <= previousTransitionId ||
            !findUnsigned(line, "material_request_id", requestId) || requestId == 0UL ||
            !findString(line, "from_status", fromStatus) ||
            !findString(line, "to_status", toStatus) ||
            !findString(line, "changed_at", changedAt) || changedAt.length() < 10U ||
            changedAt.length() > 32U ||
            !MaterialRequestStatusStore::validTransition(fromStatus, toStatus))
        {
            file.close();
            return false;
        }
        previousTransitionId = transitionId;
        if (requestId != materialRequestId) continue;
        if (fromStatus != state.status || state.transitionCount == 0xFFFFFFFFUL)
        {
            file.close();
            return false;
        }
        state.status = toStatus;
        ++state.transitionCount;
    }
    file.close();

    return state.status == "DRAFT" || state.status == "ISSUED";
}

bool MaterialRequestWarehouseCoordinator::isRemoveMutation(
    const MaterialRequestWarehousePending& pending)
{
    return pending.movementKind == "ISSUE" ||
           (pending.movementKind == "CORRECTION" &&
            pending.correctionDirection == "REMOVE");
}

String MaterialRequestWarehouseCoordinator::taggedComment(
    const String& transactionRef,
    const String& comment)
{
    String tagged = String("MRW_TX=") + transactionRef + ';';
    if (comment.length() > 0U)
    {
        tagged += ' ';
        tagged += comment;
    }
    return tagged;
}

String MaterialRequestWarehouseCoordinator::makeTransactionRef(
    uint32_t materialRequestId,
    uint32_t warehouseItemId)
{
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "MRW-%lu-%lu-%08lx-%08lx",
             static_cast<unsigned long>(materialRequestId),
             static_cast<unsigned long>(warehouseItemId),
             static_cast<unsigned long>(millis()),
             static_cast<unsigned long>(esp_random()));
    return String(buffer);
}

bool MaterialRequestWarehouseCoordinator::prepareNdjson(File& file)
{
    if (!file || file.isDirectory()) return false;
    const size_t rawSize = file.size();
    if (rawSize > 0xFFFFFFFFUL) return false;
    if (rawSize == 0U) return file.seek(0U);
    if (!file.seek(static_cast<uint32_t>(rawSize - 1U)) || file.read() != '\n')
        return false;
    return file.seek(0U);
}

bool MaterialRequestWarehouseCoordinator::findUnsigned(
    const String& line, const char* key, uint32_t& value)
{
    uint64_t parsed = 0ULL;
    if (!findUnsigned64(line, key, parsed) || parsed > 0xFFFFFFFFULL) return false;
    value = static_cast<uint32_t>(parsed);
    return true;
}

bool MaterialRequestWarehouseCoordinator::findUnsigned64(
    const String& line, const char* key, uint64_t& value)
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

bool MaterialRequestWarehouseCoordinator::findString(
    const String& line, const char* key, String& value)
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
