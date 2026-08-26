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

// RWI_TX is system-owned accounting provenance. A direct operator call to the
// generic MaterialLedger usage endpoint must not be able to spoof this prefix
// and make ordinary material usage disappear from RepairCosting.
for (const text of [
  'const String usageComment=m_server.arg("comment")',
  'usageComment.indexOf(F("RWI_TX="))==0',
  'reserved_usage_comment_prefix',
  'write_performed',
  'usage.comment=usageComment'
]) {
  requireText(materialWeb, text, `reserved RUN_WIRE ledger provenance guard ${text}`);
}

console.log('RUN_WIRE ISSUE transaction contracts: OK; RWI_TX provenance is reserved from direct MaterialLedger usage.');
