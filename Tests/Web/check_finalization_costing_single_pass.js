const fs = require('fs');
const path = require('path');

const repoRoot = path.resolve(__dirname, '..', '..');
const guardPath = path.join(repoRoot, 'firmware', 'esp32', 'src', 'CM_RepairFinalizationGuard.cpp');
const guardHeaderPath = path.join(repoRoot, 'firmware', 'esp32', 'src', 'CM_RepairFinalizationGuard.h');
const costingPath = path.join(repoRoot, 'firmware', 'esp32', 'src', 'CM_RepairCosting.cpp');
const movementPath = path.join(repoRoot, 'firmware', 'esp32', 'src', 'CM_WarehouseMovementIntegrityAudit.cpp');
const repairWebPath = path.join(repoRoot, 'firmware', 'esp32', 'src', 'CM_RepairRegistryWeb.cpp');
const deliveryWebPath = path.join(repoRoot, 'firmware', 'esp32', 'src', 'CM_RepairDeliveryWeb.cpp');
const deliveryStorePath = path.join(repoRoot, 'firmware', 'esp32', 'src', 'CM_RepairDeliveryStore.cpp');
const deliveryHeaderPath = path.join(repoRoot, 'firmware', 'esp32', 'src', 'CM_RepairDeliveryStore.h');

const guard = fs.readFileSync(guardPath, 'utf8');
const guardHeader = fs.readFileSync(guardHeaderPath, 'utf8');
const costing = fs.readFileSync(costingPath, 'utf8');
const movement = fs.readFileSync(movementPath, 'utf8');
const repairWeb = fs.readFileSync(repairWebPath, 'utf8');
const deliveryWeb = fs.readFileSync(deliveryWebPath, 'utf8');
const deliveryStore = fs.readFileSync(deliveryStorePath, 'utf8');
const deliveryHeader = fs.readFileSync(deliveryHeaderPath, 'utf8');

function requireText(source, text, description) {
  if (!source.includes(text)) {
    throw new Error(`Missing ${description}: ${text}`);
  }
}

function countText(source, text) {
  return source.split(text).length - 1;
}

if (guard.includes('WarehouseMovementIntegrityAudit::check(storage)')) {
  throw new Error('Finalization guard must not run a duplicate standalone warehouse movement audit');
}
if (guard.includes('#include "CM_WarehouseMovementIntegrityAudit.h"')) {
  throw new Error('Finalization guard must not retain an unused direct warehouse movement audit dependency');
}

requireText(
  guard,
  'RepairCosting costing(storage);',
  'finalization costing service');
requireText(
  guard,
  '? costing.loadKnownRepair(repairId, summary)',
  'known-repair costing path');
requireText(
  guard,
  ': costing.load(repairId, summary);',
  'generic authoritative costing path');
requireText(
  guardHeader,
  'static RepairFinalizationCheck checkKnownRepair(fs::FS& storage, uint32_t repairId);',
  'explicit known-repair finalization API');
requireText(
  guard,
  'return checkInternal(storage, repairId, false);',
  'generic finalization proof mode');
requireText(
  guard,
  'return checkInternal(storage, repairId, true);',
  'known-repair finalization proof mode');

const finalizationHandlerStart = repairWeb.indexOf('void RepairRegistryWeb::handleRepairFinalization()');
const closeHandlerStart = repairWeb.indexOf('void RepairRegistryWeb::handleCloseRepair()');
const parseUnsignedStart = repairWeb.indexOf('bool RepairRegistryWeb::parseUnsigned', closeHandlerStart);
if (finalizationHandlerStart < 0 || closeHandlerStart < 0 || parseUnsignedStart < 0) {
  throw new Error('Repair finalization/close handler boundaries are missing');
}
const finalizationHandler = repairWeb.slice(finalizationHandlerStart, closeHandlerStart);
const closeHandler = repairWeb.slice(closeHandlerStart, parseUnsignedStart);
for (const [name, handler] of [
  ['finalization', finalizationHandler],
  ['close', closeHandler]
]) {
  const proofIndex = handler.indexOf('m_registry.repairIsOpen(repairId, repairOpen)');
  const knownGuardIndex = handler.indexOf('RepairFinalizationGuard::checkKnownRepair(SD, repairId)');
  if (proofIndex < 0 || knownGuardIndex < 0 || proofIndex >= knownGuardIndex) {
    throw new Error(`${name} handler must prove exact repair/open state before using checkKnownRepair`);
  }
  if (handler.includes('RepairFinalizationGuard::check(SD, repairId)')) {
    throw new Error(`${name} handler must not repeat the repair journal through generic finalization check`);
  }
}
if (countText(repairWeb, 'RepairFinalizationGuard::checkKnownRepair(SD, repairId)') !== 2) {
  throw new Error('Exactly the two repair Web finalization paths must use checkKnownRepair');
}
requireText(
  closeHandler,
  'm_registry.closeRepair(repairId, m_server.arg("closed_at"), alreadyClosed)',
  'mutation-time authoritative close reread');

requireText(
  costing,
  'WarehouseMovementIntegrityAudit::checkRepair(m_storage, repairId, wireTotals)',
  'single-pass repair movement audit and aggregation');
requireText(
  movement,
  'return checkInternal(storage, ignoredRecordCount, repairId, &totals, 0UL, 0UL, nullptr);',
  'checkRepair delegation to authoritative movement audit');
requireText(
  movement,
  'if (!confirmedProvenanceUnique(storage, Path)) return false;',
  'provenance uniqueness validation');
requireText(
  movement,
  'provenanceEntriesConflict(batch[left], batch[right])',
  'within-batch provenance conflict validation');
requireText(
  movement,
  'const size_t suffixOffset = outer.position();',
  'validated batch suffix offset');
requireText(
  movement,
  '!inner.seek(static_cast<uint32_t>(suffixOffset))',
  'suffix-only later-provenance scan');

// Atomic RUN_WIRE owns both a MaterialLedger stock usage and a standard physical
// warehouse movement. Costing must publish neither a mixed in-flight snapshot nor
// count the same wire cost in both materialCostMinor and wireCostMinor.
for (const text of [
  '"/data/workshop/run-wire-issue.pending.json"',
  '"/data/workshop/run-wire-issue.pending.tmp"',
  'm_storage.exists(RunWirePendingPath)',
  'm_storage.exists(RunWirePendingTempPath)',
  'classifyRunWireManagedUsage(comment, runWireManaged)',
  'comment.indexOf(F("RWI_TX=")) != 0',
  'transactionRef.indexOf(F("RWI-")) != 0',
  'if (runWireManaged) continue;',
  'uint64_t total = summary.wireCostMinor;',
  'addChecked64(total, summary.materialCostMinor)',
  'addChecked64(total, summary.labourCostMinor)'
]) {
  requireText(costing, text, `RUN_WIRE costing dedup/fail-closed guard ${text}`);
}

// Repair delivery creation must not pre-scan the growing delivery journal and
// then immediately repeat the same scan at append. The append scan remains the
// authoritative mutation-time integrity/conflict/id-allocation boundary.
const deliveryCreateStart = deliveryWeb.indexOf('void RepairDeliveryWeb::handleCreate()');
const deliveryParseStart = deliveryWeb.indexOf('bool RepairDeliveryWeb::parseUnsigned', deliveryCreateStart);
if (deliveryCreateStart < 0 || deliveryParseStart < 0) {
  throw new Error('Repair delivery create handler boundaries are missing');
}
const deliveryCreate = deliveryWeb.slice(deliveryCreateStart, deliveryParseStart);
if (deliveryCreate.includes('m_deliveries.resolveByRepair(repairId')) {
  throw new Error('Repair delivery create must not pre-scan delivery journal before mutation-time append scan');
}
requireText(
  deliveryCreate,
  'm_deliveries.append(delivery, deliveryId, alreadyDelivered)',
  'single authoritative mutation-time delivery append scan');
requireText(
  deliveryCreate,
  '"{\\"error\\":\\"repair_already_delivered\\"}"',
  'delivery duplicate HTTP 409 semantics');
requireText(
  deliveryHeader,
  'bool append(const NewRepairDelivery& delivery,\n                uint32_t& deliveryId,\n                bool& alreadyExists);',
  'delivery append conflict-result overload');
requireText(
  deliveryStore,
  'if (!prepareAppend(delivery.repairId, deliveryId, alreadyExists)) return false;',
  'mutation-time delivery journal scan');
requireText(
  deliveryStore,
  'alreadyExists = true;\n        return true;',
  'distinct already-delivered result from authoritative scan');
requireText(
  deliveryStore,
  'return append(delivery, deliveryId, alreadyExists) && !alreadyExists;',
  'legacy two-argument append compatibility semantics');

console.log('Finalization costing single-pass contracts OK: Web finalization reuses its authoritative repair/open proof without weakening generic callers or mutation-time close rereads; warehouse provenance, RUN_WIRE costing integrity, and repair-delivery mutation-time single-pass conflict detection remain enforced.');
