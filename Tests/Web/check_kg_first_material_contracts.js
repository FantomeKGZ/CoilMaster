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
const writeOffPath = 'firmware/esp32/src/CM_WarehouseWriteOffWeb.cpp';
const coveragePath = 'firmware/esp32/src/CM_WireWriteOffCoverageAudit.cpp';
const quantity = read(quantityPath);
const record = read(recordPath);
const writeOff = read(writeOffPath);
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

// Existing production write-off remains manual and exact-run protected while
// kg-first storage/API migration is implemented incrementally.
for (const text of ['source_session_id', 'source_run_id', 'confirmedWriteOffForSourceRun(sourceSessionId,']) {
  requireText(writeOffPath, writeOff, text, 'manual exact-run guard missing: ' + text);
}
requireText(coveragePath, coverage, '\\"event\\":\\"RUN_COMPLETED\\"',
  'finalization coverage no longer anchors to completed runs');

// Do not allow this migration to accidentally introduce automatic deduction.
for (const forbidden of ['automaticWriteOff(', 'autoWriteOff(', 'writeOffOnRunCompleted(']) {
  if (writeOff.includes(forbidden) || coverage.includes(forbidden)) {
    failures.push('kg-first migration introduced automatic write-off hook: ' + forbidden);
  }
}

if (failures.length) {
  console.error(failures.join('\n'));
  process.exit(1);
}

console.log('KG-first material contracts OK: exact decimal kg parser, dual movement schema, exact source-run provenance, and no automatic RUN_COMPLETED deduction.');
