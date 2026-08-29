const fs = require('fs');
const path = require('path');

const repoRoot = path.resolve(__dirname, '..', '..');
const guardPath = path.join(repoRoot, 'firmware', 'esp32', 'src', 'CM_RepairFinalizationGuard.cpp');
const costingPath = path.join(repoRoot, 'firmware', 'esp32', 'src', 'CM_RepairCosting.cpp');
const movementPath = path.join(repoRoot, 'firmware', 'esp32', 'src', 'CM_WarehouseMovementIntegrityAudit.cpp');

const guard = fs.readFileSync(guardPath, 'utf8');
const costing = fs.readFileSync(costingPath, 'utf8');
const movement = fs.readFileSync(movementPath, 'utf8');

function requireText(source, text, description) {
  if (!source.includes(text)) {
    throw new Error(`Missing ${description}: ${text}`);
  }
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
  'if (!costing.load(repairId, summary))',
  'authoritative costing load');
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

console.log('Finalization costing single-pass contracts OK: authoritative movement transaction/provenance validation and repair aggregation share the costing pass; provenance batches validate pairs once and scan only the later journal suffix; atomic RUN_WIRE ledger usage is not double-counted and in-flight RUN_WIRE costing fails closed.');
