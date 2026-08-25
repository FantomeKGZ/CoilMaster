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

console.log('Finalization costing single-pass contracts OK: authoritative movement transaction/provenance validation and repair aggregation share the costing pass.');
