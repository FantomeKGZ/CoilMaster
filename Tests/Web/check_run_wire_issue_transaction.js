const fs = require('fs');

function read(path) {
  return fs.readFileSync(path, 'utf8');
}

function requireText(source, text, label) {
  if (!source.includes(text)) {
    throw new Error(`Missing ${label}: ${text}`);
  }
}

function requireOrder(source, labels) {
  let previous = -1;
  for (const [label, text] of labels) {
    const index = source.indexOf(text);
    if (index < 0) throw new Error(`Missing ordered contract ${label}: ${text}`);
    if (index <= previous) throw new Error(`Invalid order at ${label}`);
    previous = index;
  }
}

const pendingHeader = read('firmware/esp32/src/CM_RunWireIssuePendingStore.h');
const coordinator = read('firmware/esp32/src/CM_RunWireIssueCoordinator.cpp');
const web = read('firmware/esp32/src/CM_MaterialRequestWeb.cpp');
const managedWarehouse = read('firmware/esp32/src/CM_WarehouseRunWireManaged.cpp');
const runtime = read('firmware/esp32/src/CM_MaterialRequestRuntime.cpp');
const materialWeb = read('firmware/esp32/src/CM_MaterialLedgerWeb.cpp');
const directWriteoffWeb = read('firmware/esp32/src/CM_WarehouseWriteOffWeb.cpp');
const directWriteoffStore = read('firmware/esp32/src/CM_WarehouseWriteOff.cpp');
const accountingAudit = read('firmware/esp32/src/CM_RunWireAccountingIntegrityAudit.cpp');
const workshopAudit = read('firmware/esp32/src/CM_WorkshopPersistenceIntegrityAudit.cpp');

for (const field of [
  'materialRequestId', 'repairId', 'warehouseItemId',
  'sourceSessionId', 'sourceRunId', 'spoolId', 'consumedGrams',
  'spoolWeightBeforeGrams', 'spoolWeightAfterGrams',
  'ledgerQuantityMilli', 'materialClass', 'wireDiameterHundredthsMm'
]) {
  requireText(pendingHeader, field, `authoritative pending field ${field}`);
}

requireText(coordinator, 'requestedMovement.movementKind != "ISSUE"', 'RUN_WIRE ISSUE-only guard');
requireText(coordinator, 'requestedMovement.sourceKind != "RUN_WIRE"', 'RUN_WIRE source guard');
requireText(coordinator, 'requestedMovement.unit != "KG"', 'KG-only guard');
requireText(coordinator, 'JobSpoolSelectionStore::loadReadOnly', 'immutable exact spool selection');
requireText(coordinator, 'WindingSessionCompletionAudit::check', 'exact completed run requirement');
requireText(coordinator, 'm_bridges.loadBySpool', 'spool material bridge requirement');
requireText(coordinator, 'item.hasWireMetadata', 'MaterialLedger wire metadata requirement');
requireText(coordinator, 'item.wireType != bridge.wireType', 'material class bridge match');
requireText(coordinator, 'item.diameterHundredthsMm != bridge.diameterHundredthsMm', 'diameter bridge match');
requireText(coordinator, 'm_warehouse.confirmedWriteOffForSourceRun', 'duplicate exact source-run protection');

requireOrder(coordinator, [
  ['authoritative pending', 'm_pending.save(pending)'],
  ['material request movement', 'appendMaterialRequestMovement(pending, materialRequestMovementId)'],
  ['ledger usage', 'applyLedger(pending, remainingLedgerQuantityMilli)'],
  ['physical phases', 'executePhysicalPhases(pending)'],
  ['pending clear', 'm_pending.clear()']
]);

requireText(coordinator, 'RWI_TX=', 'shared transaction tag');
requireText(coordinator, 'warehouseEvidenceExists', 'warehouse evidence recovery');
requireText(coordinator, 'spoolStateMatches', 'exact before/after recovery evidence');
requireText(coordinator, '!movementFound && (ledgerFound || warehouseFound || spoolAtAfter)', 'fail-closed impossible ordering');
requireText(coordinator, 'movementFound && !ledgerFound && (warehouseFound || spoolAtAfter)', 'fail-closed ledger ordering');

// RUN_WIRE must not begin with one MaterialLedger price and later commit a
// different physical warehouse price. buildPending runs before pending.save.
const buildPendingStart = coordinator.indexOf('bool RunWireIssueCoordinator::buildPending(');
const appendMovementStart = coordinator.indexOf('bool RunWireIssueCoordinator::appendMaterialRequestMovement(', buildPendingStart);
const buildPendingBody = coordinator.slice(buildPendingStart, appendMovementStart);
for (const text of [
  'm_warehouse.loadWarehousePrice(warehousePrice, warehousePriceConfigured)',
  'warehousePrice.currency != item.currency',
  'conversion.requestUnitCostMinor > 0xFFFFFFFFULL',
  'warehousePrice.pricePerKgMinor !=',
  'static_cast<uint32_t>(conversion.requestUnitCostMinor)'
]) {
  requireText(buildPendingBody, text, `pre-pending price convergence ${text}`);
}
requireText(coordinator, 'if (!buildPending(requestedMovement, spoolId, pending) ||', 'price-checked build before pending');
requireText(coordinator, '!m_pending.save(pending)', 'pending persisted only after build');

const physicalStart = coordinator.indexOf('bool RunWireIssueCoordinator::executePhysicalPhases(');
const movementEvidenceStart = coordinator.indexOf('bool RunWireIssueCoordinator::movementEvidenceExists(', physicalStart);
const physicalBody = coordinator.slice(physicalStart, movementEvidenceStart);
for (const text of [
  'm_warehouse.loadWarehousePrice(warehousePrice, warehousePriceConfigured)',
  'warehousePrice.currency != pending.currency',
  'pending.unitCostMinor > 0xFFFFFFFFULL',
  'warehousePrice.pricePerKgMinor != static_cast<uint32_t>(pending.unitCostMinor)'
]) {
  requireText(physicalBody, text, `recovery/physical price convergence ${text}`);
}

requireText(managedWarehouse, 'appendKgFirstWriteOffRecord', 'existing append-only warehouse evidence');
requireText(managedWarehouse, '"PENDING"', 'subordinate warehouse pending phase');
requireText(managedWarehouse, '"CONFIRMED"', 'subordinate warehouse confirmed phase');
requireText(managedWarehouse, 'identity.currentWeightGrams == weightAfterGrams', 'idempotent physical replay');
requireText(managedWarehouse, 'identity.currentWeightGrams != weightBeforeGrams', 'unexpected spool state rejection');

requireText(web, 'movement.sourceKind == "RUN_WIRE"', 'dedicated RUN_WIRE branch');
requireText(web, 'movement.movementKind != "ISSUE"', 'web ISSUE-only guard');
requireText(web, 'movement.unit != "KG"', 'web KG-only guard');
requireText(web, '"spool_id"', 'explicit exact spool_id input');
requireText(web, 'm_runWire->execute(movement, spoolId, result)', 'dedicated coordinator execution');

const runWireBranch = web.indexOf('if (movement.sourceKind == "RUN_WIRE")');
const dedicatedExecute = web.indexOf('m_runWire->execute(movement, spoolId, result)', runWireBranch);
const dedicatedReturn = web.indexOf('return;', dedicatedExecute);
const genericExecute = web.indexOf('m_warehouse.execute(movement, correctionDirection, result)', dedicatedReturn);
if (runWireBranch < 0 || dedicatedExecute < 0 || dedicatedReturn < 0 || genericExecute < 0 ||
    !(runWireBranch < dedicatedExecute && dedicatedExecute < dedicatedReturn && dedicatedReturn < genericExecute)) {
  throw new Error('RUN_WIRE must return before generic Ledger-only coordinator path');
}

requireText(runtime, 'RunWireIssuePendingStore runWirePending', 'runtime authoritative pending');
requireText(runtime, 'SpoolMaterialBridgeStore spoolMaterialBridges', 'runtime bridge store');
requireText(runtime, 'RunWireIssueCoordinator runWire', 'runtime dedicated coordinator');
requireText(runtime, '!runWire.begin()', 'runtime fail-closed recovery gate');

// RWI_TX is system-owned accounting provenance. Generic material usage and the
// compatibility direct writeoff endpoint must not be able to spoof it.
for (const text of [
  'const String usageComment=m_server.arg("comment")',
  'usageComment.indexOf(F("RWI_TX="))==0',
  'reserved_usage_comment_prefix',
  'write_performed',
  'usage.comment=usageComment'
]) {
  requireText(materialWeb, text, `reserved RUN_WIRE ledger provenance guard ${text}`);
}
for (const text of [
  'const String operatorComment = m_server.arg("comment")',
  'operatorComment.indexOf(F("RWI_TX=")) == 0',
  'reserved_writeoff_comment_prefix',
  'operation.comment = operatorComment'
]) {
  requireText(directWriteoffWeb, text, `reserved RUN_WIRE warehouse provenance guard ${text}`);
}

// Compatibility direct writeoff and atomic RUN_WIRE must share the same exact
// session+run duplicate authority. This makes path switching non-repeatable:
// atomic after legacy and legacy after atomic both stop before physical mutation.
for (const [source, label] of [
  [coordinator, 'atomic RUN_WIRE'],
  [directWriteoffWeb, 'legacy/direct Web'],
  [directWriteoffStore, 'legacy/direct store']
]) {
  requireText(source, 'confirmedWriteOffForSourceRun', `${label} exact-run duplicate lookup`);
  requireText(source, 'alreadyConfirmed', `${label} exact-run duplicate result`);
}
requireText(
  coordinator,
  'm_warehouse.confirmedWriteOffForSourceRun(requestedMovement.sourceSessionId,',
  'atomic exact source-session duplicate key');
requireText(
  coordinator,
  'requestedMovement.sourceRunId,',
  'atomic exact source-run duplicate key');
requireText(
  directWriteoffWeb,
  'm_store.confirmedWriteOffForSourceRun(sourceSessionId, sourceRunId, alreadyConfirmed)',
  'direct Web exact session+run duplicate key');
requireText(
  directWriteoffWeb,
  'source_run_already_written_off',
  'direct Web explicit duplicate rejection');
requireText(
  directWriteoffStore,
  'confirmedWriteOffForSourceRun(operation.sourceSessionId,',
  'direct store exact source-session duplicate key');
requireText(
  directWriteoffStore,
  'operation.sourceRunId,',
  'direct store exact source-run duplicate key');

// Completed RUN_WIRE accounting is a three-log invariant. The audit is bounded,
// read-only and refuses to interpret an in-flight high-level transaction.
for (const text of [
  'ReferenceBatchSize = 16U',
  'RunWireIssuePendingStore::Path',
  'RunWireIssuePendingStore::TempPath',
  'JobSpoolSelectionStore::loadReadOnly',
  'SpoolMaterialBridgeStore::Path',
  'resolveLedgerBatch',
  'resolveWarehouseBatch',
  'WarehouseWriteOffMode::KgFirst',
  'WarehouseWriteOffStockMode::Spool',
  'reference.spoolId',
  'reference.warehouseItemId',
  'reference.sourceSessionId',
  'reference.sourceRunId',
  'reference.consumedGrams',
  'reference.ledgerQuantityMilli',
  'reference.unitCostMinor',
  'reference.materialClass',
  'reference.diameterHundredthsMm',
  'price_per_unit_minor',
  'record.pricePerKgMinor != static_cast<uint32_t>(reference.unitCostMinor)',
  'metrics.movementCount == metrics.ledgerEvidenceCount',
  'metrics.movementCount == metrics.warehouseEvidenceCount'
]) {
  requireText(accountingAudit, text, `cross-log RUN_WIRE accounting contract ${text}`);
}
requireText(accountingAudit, 'reference.unitCostMinor / 1000ULL', 'Ledger gram price from immutable KG price');
requireText(accountingAudit, 'extractRunWireTag', 'system RWI transaction tag parser');
requireText(accountingAudit, 'RWI_TX=', 'cross-log transaction marker');
requireText(accountingAudit, 'matches > 1U', 'exact bridge duplicate rejection');
requireText(accountingAudit, 'ledgerMatches > 1U', 'duplicate ledger evidence rejection');
requireText(accountingAudit, 'warehouseMatches > 1U', 'duplicate warehouse evidence rejection');
requireText(workshopAudit, '#include "CM_RunWireAccountingIntegrityAudit.h"', 'workshop cross-log audit include');
requireText(workshopAudit, '!RunWireAccountingIntegrityAudit::check(storage)', 'workshop cross-log fail-closed gate');

console.log('RUN_WIRE ISSUE transaction contracts: OK; accounting identity, system provenance and one KG wire price converge across Material Request, MaterialLedger and warehouse CONFIRMED evidence.');
