const fs = require('fs');
const path = require('path');

const root = path.resolve(__dirname, '../..');
const relative = 'firmware/esp32/src/CM_RepairFinalizationGuard.cpp';
const coverageRelative = 'firmware/esp32/src/CM_WireWriteOffCoverageAudit.cpp';
const movementAuditRelative = 'firmware/esp32/src/CM_WarehouseMovementIntegrityAudit.cpp';
const movementHeaderRelative = 'firmware/esp32/src/CM_WarehouseMovementIntegrityAudit.h';
const source = fs.readFileSync(path.join(root, relative), 'utf8');
const coverage = fs.readFileSync(path.join(root, coverageRelative), 'utf8');
const movementAudit = fs.readFileSync(path.join(root, movementAuditRelative), 'utf8');
const movementHeader = fs.readFileSync(path.join(root, movementHeaderRelative), 'utf8');
const failures = [];

for (const required of [
  '#include "CM_WindingJournalTransitionAudit.h"',
  'WindingJournalTransitionAudit::validate(storage)',
  'RepairFinalizationCheck::WindingStorageUnavailable',
  'RepairFinalizationCheck::WindingIntegrityFailed',
  'WireWriteOffCoverageAudit::check(storage, repairId)'
]) {
  if (!source.includes(required)) failures.push(relative + ': required finalization invariant missing: ' + required);
}

for (const forbidden of [
  '#include "CM_WindingJournalQuery.h"',
  'WindingJournalQuery history(storage)',
  'history.validateAll()',
  'query.validateAll()'
]) {
  if (source.includes(forbidden)) failures.push(relative + ': redundant winding journal pre-scan returned: ' + forbidden);
}

for (const required of [
  'constexpr uint8_t CoverageBatchSize = 32U;',
  'static_assert(CoverageBatchSize == WarehouseMovementCoverageMaxTargets,',
  'using CoverageTarget = WarehouseMovementCoverageTarget;',
  'bool movementIntegrityValidated = false;',
  'CoverageTarget targets[CoverageBatchSize];',
  '? WarehouseMovementIntegrityAudit::checkCoverageBatch(',
  ': confirmedWriteOffBatch(storage, repairId, targets, targetCount);',
  'movementIntegrityValidated = true;'
]) {
  if (!coverage.includes(required)) failures.push(coverageRelative + ': fused first coverage pass invariant missing: ' + required);
}
if (coverage.includes('WarehouseMovementIntegrityAudit::check(storage)')) {
  failures.push(coverageRelative + ': redundant standalone movement integrity pre-scan returned');
}

for (const required of [
  'constexpr uint8_t WarehouseMovementCoverageMaxTargets = 32U;',
  'struct WarehouseMovementCoverageTarget',
  'static bool checkCoverageBatch(fs::FS& storage,'
]) {
  if (!movementHeader.includes(required)) failures.push(movementHeaderRelative + ': audited coverage batch API missing: ' + required);
}
for (const required of [
  'bool accumulateCoverageRecord(const WarehouseWriteOffRecord& record,',
  'record.repairId != repairId',
  'record.spoolId != target.spoolId',
  'record.sourceRunId != target.runId',
  'coverageTargets != nullptr &&',
  '!accumulateCoverageRecord(record, repairId,',
  'if (!confirmedProvenanceUnique(storage, Path)) return false;',
  'bool WarehouseMovementIntegrityAudit::checkCoverageBatch('
]) {
  if (!movementAudit.includes(required)) failures.push(movementAuditRelative + ': authoritative coverage aggregation invariant missing: ' + required);
}

if (failures.length) {
  console.error(failures.join('\n'));
  process.exit(1);
}

console.log('Finalization winding contracts OK: winding validation stays single-pass and the first bounded write-off coverage scan is fused into authoritative movement pairing/provenance validation.');
