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
const quantity = read(quantityPath);
const record = read(recordPath);
const store = read(storePath);
const writeOff = read(writeOffPath);
const recovery = read(recoveryPath);
const coverage = read(coveragePath);

// kg-first quantities must be converted deterministically to integer grams;
// floating-point parsing is forbidden in this accounting boundary.
for (const text of ['static bool parseGrams', 'fractionalDigits > 3', 'parsed == 0UL', 'static String canonicalKg']) {
  requireText(quantityPath, quantity, text, 'exact kg quantity contract missing: ' + text);
}
for (const forbidden of ['toFloat(', 'atof(', 'strtod(', 'double ', 'float ']) {
  if (quantity.includes(forbidden)) failures.push(quantityPath + ': floating-point quantity parsing is forbidden: ' + forbidden);
}

// The append-only movement journal must distinguish legacy exact-spool records
// from kg-first records explicitly. Unallocated consumption may omit spool and
// weight fields, but exact source session/run and conductor snapshot stay required.
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

// The production endpoint now accepts explicit KG_FIRST requests. quantity_kg
// is mandatory; spool_id is optional only in that mode, and unallocated writes
// must carry a conductor snapshot.
for (const text of [
  'm_server.arg("writeoff_mode") == "KG_FIRST"',
  'KgQuantity::parseGrams(m_server.arg("quantity_kg"), consumedGrams)',
  'm_server.hasArg("spool_id")',
  'diameter_required_for_unallocated',
  'wire_type_required_for_unallocated',
  'confirmKgFirstWriteOff(operation, result)',
  'source_session_id',
  'source_run_id',
  'confirmedWriteOffForSourceRun(sourceSessionId, sourceRunId, alreadyConfirmed)'
]) {
  requireText(writeOffPath, writeOff, text, 'active kg-first API guard missing: ' + text);
}

// Store-level safety remains authoritative even if HTTP validation changes.
for (const text of [
  'bool WarehouseStore::confirmKgFirstWriteOff',
  'WindingSessionCompletionAudit::check',
  'alreadyConfirmed',
  'operation.consumedGrams >= identity.currentWeightGrams',
  'appendKgFirstWriteOffRecord',
  'writeoff_mode',
  'KG_FIRST',
  'UNALLOCATED'
]) {
  requireText(storePath, store, text, 'kg-first store invariant missing: ' + text);
}

// Recovery must never invent an unallocated write-off after reboot. Stock-backed
// transactions may be confirmed only when the durable spool state proves it.
for (const text of [
  'WarehouseWriteOffStockMode::Unallocated',
  '"ABORTED"',
  'currentWeight == pending.weightBeforeGrams',
  'currentWeight == pending.weightAfterGrams'
]) {
  requireText(recoveryPath, recovery, text, 'kg-first recovery invariant missing: ' + text);
}

// Finalization remains anchored to completed runs. Legacy records retain exact
// spool matching; explicit UNALLOCATED kg-first records may cover the exact run
// without pretending that warehouse stock was mutated.
requireText(coveragePath, coverage, '\\"event\\":\\"RUN_COMPLETED\\"',
  'finalization coverage no longer anchors to completed runs');
for (const text of [
  'record.mode == WarehouseWriteOffMode::LegacySpool',
  'record.spoolId != spoolId',
  'record.stockMode == WarehouseWriteOffStockMode::Unallocated',
  'record.sourceRunId != runId'
]) {
  requireText(coveragePath, coverage, text, 'finalization kg-first/legacy coverage split missing: ' + text);
}

// Do not allow this migration to accidentally introduce automatic deduction.
for (const forbidden of ['automaticWriteOff(', 'autoWriteOff(', 'writeOffOnRunCompleted(']) {
  if (store.includes(forbidden) || writeOff.includes(forbidden) || recovery.includes(forbidden) || coverage.includes(forbidden)) {
    failures.push('kg-first migration introduced automatic write-off hook: ' + forbidden);
  }
}

if (failures.length) {
  console.error(failures.join('\n'));
  process.exit(1);
}

console.log('KG-first material contracts OK: exact decimal kg parser, active manual API, dual journal schema, reboot recovery, exact source-run provenance, and no automatic RUN_COMPLETED deduction.');
