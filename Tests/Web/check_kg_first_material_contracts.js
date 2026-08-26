const fs = require('fs');
const path = require('path');

const root = path.resolve(__dirname, '../..');
const failures = [];

function read(relative) {
  return fs.readFileSync(path.join(root, relative), 'utf8');
}

function requireText(relative, source, text, description) {
  if (!source.includes(text)) failures.push(relative + ': ' + description);
}

const quantityPath = 'firmware/esp32/src/CM_KgQuantity.h';
const recordPath = 'firmware/esp32/src/CM_WarehouseWriteOffRecord.h';
const storePath = 'firmware/esp32/src/CM_WarehouseWriteOff.cpp';
const writeOffPath = 'firmware/esp32/src/CM_WarehouseWriteOffWeb.cpp';
const recoveryPath = 'firmware/esp32/src/CM_WarehouseWriteOffRecovery.cpp';
const coveragePath = 'firmware/esp32/src/CM_WireWriteOffCoverageAudit.cpp';
const costingPath = 'firmware/esp32/src/CM_RepairCosting.cpp';
const movementAuditPath = 'firmware/esp32/src/CM_WarehouseMovementIntegrityAudit.cpp';
const movementAuditHeaderPath = 'firmware/esp32/src/CM_WarehouseMovementIntegrityAudit.h';
const warehousePersistencePath = 'firmware/esp32/src/CM_WarehousePersistenceIntegrityAudit.cpp';
const completionAuditPath = 'firmware/esp32/src/CM_WindingSessionCompletionAudit.cpp';
const transitionAuditPath = 'firmware/esp32/src/CM_WindingJournalTransitionAudit.cpp';
const controllerPath = 'firmware/esp32/web/shared/writeoff-spool-suggestion.js';
const desktopPath = 'firmware/esp32/web/desktop/writeoff.html';
const mobilePath = 'firmware/esp32/web/mobile/writeoff.html';
const quantity = read(quantityPath);
const record = read(recordPath);
const store = read(storePath);
const writeOff = read(writeOffPath);
const recovery = read(recoveryPath);
const coverage = read(coveragePath);
const costing = read(costingPath);
const movementAudit = read(movementAuditPath);
const movementAuditHeader = read(movementAuditHeaderPath);
const warehousePersistence = read(warehousePersistencePath);
const completionAudit = read(completionAuditPath);
const transitionAudit = read(transitionAuditPath);
const controller = read(controllerPath);
const desktop = read(desktopPath);
const mobile = read(mobilePath);

for (const text of ['static bool parseGrams', 'fractionalDigits > 3', 'parsed == 0UL', 'static String canonicalKg']) {
  requireText(quantityPath, quantity, text, 'exact kg quantity contract missing: ' + text);
}
for (const forbidden of ['toFloat(', 'atof(', 'strtod(', 'double ', 'float ']) {
  if (quantity.includes(forbidden)) failures.push(quantityPath + ': floating-point quantity parsing is forbidden: ' + forbidden);
}

for (const text of [
  'WarehouseWriteOffMode::KgFirst',
  '"writeoff_mode"',
  'mode != "KG_FIRST"',
  'stockMode == "SPOOL"',
  'stockMode == "UNALLOCATED"',
  '!record.hasSourceSessionId || !record.hasSourceRunId',
  'record.diameterHundredthsMm == 0U || !record.hasWireType',
  'parsedGrams != record.massGrams',
  'KgQuantity::canonicalKg(parsedGrams) != record.quantityKg',
  '!record.hasSpoolId && !record.hasWeights'
]) {
  requireText(recordPath, record, text, 'kg-first journal shape guard missing: ' + text);
}

for (const text of [
  'm_server.arg("writeoff_mode") == "KG_FIRST"',
  'KgQuantity::parseGrams(m_server.arg("quantity_kg"), consumedGrams)',
  'parseUnsignedArg(m_server, "spool_id", 1UL, 0xFFFFFFFFUL, spoolId)',
  'spool_id_required_for_kg_first',
  'selection.repairId != repairId || selection.spoolId != spoolId',
  'confirmKgFirstWriteOff(operation, result)',
  'source_session_id',
  'source_run_id',
  'confirmedWriteOffForSourceRun(sourceSessionId, sourceRunId, alreadyConfirmed)',
  'response += F(",\\\"stock_mode\\\":\\\"SPOOL\\\"")'
]) {
  requireText(writeOffPath, writeOff, text, 'active exact-spool kg-first API guard missing: ' + text);
}
for (const forbidden of ['diameter_required_for_unallocated', 'wire_type_required_for_unallocated', '(spoolId != 0UL && selection.spoolId != spoolId)']) {
  if (writeOff.includes(forbidden)) failures.push(writeOffPath + ': new POST path still exposes legacy unallocated fallback: ' + forbidden);
}

for (const text of [
  'bool WarehouseStore::confirmKgFirstWriteOff',
  'WindingSessionCompletionAudit::check',
  'alreadyConfirmed',
  'operation.spoolId == 0UL || selection.spoolId != operation.spoolId',
  'operation.consumedGrams >= identity.currentWeightGrams',
  'appendKgFirstWriteOffRecord',
  'writeoff_mode',
  'KG_FIRST',
  'UNALLOCATED'
]) {
  requireText(storePath, store, text, 'kg-first store invariant missing: ' + text);
}

for (const text of [
  'WarehouseWriteOffStockMode::Unallocated',
  '"ABORTED"',
  'currentWeight == pending.weightBeforeGrams',
  'currentWeight == pending.weightAfterGrams'
]) {
  requireText(recoveryPath, recovery, text, 'kg-first recovery invariant missing: ' + text);
}

requireText(coveragePath, coverage, '\\"event\\":\\"RUN_COMPLETED\\"',
  'finalization coverage no longer anchors to completed runs');
for (const text of [
  'record.mode == WarehouseWriteOffMode::LegacySpool',
  'record.spoolId != target.spoolId',
  'if (!record.hasSourceRunId) return false;',
  'matches = record.sourceRunId == target.runId;',
  'record.stockMode == WarehouseWriteOffStockMode::Unallocated',
  'record.sourceRunId != target.runId',
  'selectionCatalog == ReadOnlyCatalogCheck::Missing',
  'if (!selectionFound)',
  'return WireWriteOffCoverageCheck::IntegrityFailed;'
]) {
  requireText(coveragePath, coverage, text, 'finalization exact-run/selection coverage guard missing: ' + text);
}
for (const text of [
  'constexpr uint8_t CoverageBatchSize = 32U;',
  'CoverageTarget targets[CoverageBatchSize];',
  'confirmedWriteOffBatch(storage, repairId, targets, targetCount)',
  'history.appendHistoryJson(0UL, repairId, cursor, CoverageBatchSize',
  'for (uint8_t i = 0U; i < targetCount; ++i)'
]) {
  requireText(coveragePath, coverage, text, 'batched finalization coverage guard missing: ' + text);
}
for (const forbidden of [
  'if (selectionCatalog == ReadOnlyCatalogCheck::Missing) continue;',
  'if (!selectionFound) continue;',
  'bool confirmedWriteOffExists('
]) {
  if (coverage.includes(forbidden)) failures.push(coveragePath + ': closure coverage bypass returned: ' + forbidden);
}

for (const text of [
  'struct WarehouseMovementRepairTotals',
  'static bool checkRepair(fs::FS& storage',
  'uint64_t wireCostMinor',
  'uint32_t copperWireGrams',
  'uint16_t wireLineCount'
]) {
  requireText(movementAuditHeaderPath, movementAuditHeader, text, 'movement repair aggregation API missing: ' + text);
}
for (const text of [
  'accumulateRepairRecord(record, repairId, *totals)',
  'record.status != "CONFIRMED" || record.repairId != repairId',
  '(product + 500ULL) / 1000ULL',
  'totals.currency != record.currency',
  'record.wireType == "CU"',
  'record.wireType == "AL"'
]) {
  requireText(movementAuditPath, movementAudit, text, 'single-pass repair wire aggregation guard missing: ' + text);
}
for (const text of [
  'WarehouseMovementRepairTotals wireTotals;',
  'WarehouseMovementIntegrityAudit::checkRepair(m_storage, repairId, wireTotals)',
  'summary.wireCostMinor = wireTotals.wireCostMinor;',
  'summary.copperWireGrams = wireTotals.copperWireGrams;',
  'bool currencySet = wireTotals.currencySet;'
]) {
  requireText(costingPath, costing, text, 'costing must consume audited movement totals: ' + text);
}
for (const forbidden of [
  'WarehouseWriteOffRecordCodec::parse(line, record)',
  'm_storage.open(WireMovementsPath, FILE_READ)',
  '#include "CM_WarehouseWriteOffRecord.h"',
  'pendingSpoolId',
  '!findUnsigned(line, "spool_id", spoolId)',
  '!findUnsigned(line, "weight_before_g", before)',
  '!findUnsigned(line, "weight_after_g", after)'
]) {
  if (costing.includes(forbidden)) failures.push(costingPath + ': redundant/legacy wire costing scan returned: ' + forbidden);
}

for (const text of [
  'constexpr uint8_t BatchSize = 32U;',
  'ProvenanceEntry batch[BatchSize];',
  'while (outer.available() && batchCount < BatchSize)',
  'for (uint8_t index = 0U; index < batchCount; ++index)',
  'provenanceConflicts(batch[index], candidate)',
  'candidate.sourceRunId == entry.sourceRunId'
]) {
  requireText(movementAuditPath, movementAudit, text, 'batched provenance audit guard missing: ' + text);
}
if (movementAudit.includes('candidate.movementId == record.movementId')) {
  failures.push(movementAuditPath + ': per-record provenance rescan implementation returned');
}

for (const text of [
  '#include "CM_WarehouseWriteOffRecord.h"',
  'constexpr uint8_t ReferenceBatchSize = 32U;',
  'WarehouseWriteOffRecordCodec::parse(line, record)',
  'repairReferences[repairCount].id = record.repairId;',
  'if (record.hasSpoolId)',
  'spoolReferences[spoolCount].id = record.spoolId;',
  'validateReferenceBatch(storage,',
  'resolveReferences(storage, RepairsPath, "repair_id"',
  'resolveReferences(storage, SpoolsPath, "spool_id"'
]) {
  requireText(warehousePersistencePath, warehousePersistence, text, 'kg-first backup warehouse audit guard missing: ' + text);
}
for (const forbidden of [
  'bool idExists(',
  '!idExists(storage, SpoolsPath',
  '!idExists(storage, RepairsPath'
]) {
  if (warehousePersistence.includes(forbidden)) {
    failures.push(warehousePersistencePath + ': legacy per-record backup reference audit returned: ' + forbidden);
  }
}

for (const text of [
  'query.validateAll()',
  'WindingJournalTransitionAudit::validate(storage, sessionId, runId, completed)',
  'return completed',
]) {
  requireText(completionAuditPath, completionAudit, text, 'two-pass completion audit guard missing: ' + text);
}
if (completionAudit.includes('appendHistoryJson(') || completionAudit.includes('pageContainsCompletedRun')) {
  failures.push(completionAuditPath + ': redundant third winding-journal scan returned');
}
for (const text of [
  'bool* completed',
  'targetRunId == 0UL || runId == targetRunId',
  'sessionId == targetSessionId',
  'return validateInternal(storage, sessionId, runId, &completed);'
]) {
  requireText(transitionAuditPath, transitionAudit, text, 'transition completion evidence guard missing: ' + text);
}

for (const [relative, source] of [[desktopPath, desktop], [mobilePath, mobile]]) {
  for (const text of ['Количество, кг', 'id="quantityKg"', 'id="materialRequestId"', 'id="materialRequestInfo"', 'id="allocationMode"', 'value="SPOOL"', 'id="wireType"', 'id="diameterMm"', 'Подтвердить RUN_WIRE списание', '/shared/writeoff-spool-suggestion.js']) {
    requireText(relative, source, text, 'atomic RUN_WIRE writeoff UI missing: ' + text);
  }
  for (const forbidden of ['id="before"', 'id="after"', 'Вес до работы', 'Вес после работы', 'value="UNALLOCATED"']) {
    if (source.includes(forbidden)) failures.push(relative + ': production form exposes obsolete writeoff control: ' + forbidden);
  }
}

for (const text of [
  "'/api/material-requests/warehouse'",
  "confirmed:'true'",
  "material_request_id:selectedMaterialRequestId",
  "warehouse_item_id:String(spoolBridge.warehouse_item_id)",
  "quantity_milli_units:String(quantity.grams)",
  "movement_kind:'ISSUE'",
  "source_kind:'RUN_WIRE'",
  "unit:'KG'",
  "source_session_id:sourceSessionId",
  "source_run_id:sourceRunId",
  "spool_id:String(activeSpool.spool_id)",
  "wire_diameter_hundredths_mm:String(activeSpool.diameter_hundredths_mm)",
  "material_class:String(activeSpool.material_class)",
  "'/api/warehouse/spool-material-bridges?spool_id='",
  "'/api/material-requests?'+q",
  "'/api/material-requests/status?material_request_id='",
  "status.status==='DRAFT'||status.status==='ISSUED'",
  "event.event!=='RUN_COMPLETED'",
  "found.material_class==='CU'||found.material_class==='AL'",
  "менять provenance после RUN_COMPLETED нельзя"
]) {
  requireText(controllerPath, controller, text, 'atomic RUN_WIRE UI controller contract missing: ' + text);
}
for (const forbidden of [
  "jsonFetch('/api/warehouse/write-offs',{method:'POST'",
  "writeoff_mode:'KG_FIRST'",
  "quantity_kg:quantity.kg",
  "body.set('diameter_hundredths_mm'",
  "body.set('wire_type'",
  'Используйте списание без привязки',
  '<option value="UNALLOCATED">'
]) {
  if (controller.includes(forbidden)) failures.push(controllerPath + ': production operator UI must not use legacy/direct writeoff path: ' + forbidden);
}

for (const forbidden of ['automaticWriteOff(', 'autoWriteOff(', 'writeOffOnRunCompleted(']) {
  if (store.includes(forbidden) || writeOff.includes(forbidden) || recovery.includes(forbidden) || coverage.includes(forbidden) || controller.includes(forbidden)) {
    failures.push('kg-first migration introduced automatic write-off hook: ' + forbidden);
  }
}

if (failures.length) {
  console.error(failures.join('\n'));
  process.exit(1);
}

console.log('KG-first material contracts OK: exact kg accounting, historical dual journal compatibility, atomic operator RUN_WIRE through Material Request, exact bridge/session/run/spool provenance, batched runtime and backup scans, audited costing, exact-run finalization, historical unallocated rendering/recovery, and no automatic RUN_COMPLETED deduction.');
