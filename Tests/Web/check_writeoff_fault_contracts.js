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
function requireAbsent(relative, source, text, description) {
  if (source.includes(text)) failures.push(relative + ': ' + description);
}

const recoveryPath = 'firmware/esp32/src/CM_WarehouseWriteOffRecovery.cpp';
const storePath = 'firmware/esp32/src/CM_WarehouseStore.cpp';
const storeHeaderPath = 'firmware/esp32/src/CM_WarehouseStore.h';
const writeoffPath = 'firmware/esp32/src/CM_WarehouseWriteOffWeb.cpp';
const writeoffStorePath = 'firmware/esp32/src/CM_WarehouseWriteOff.cpp';
const writeoffLookupPath = 'firmware/esp32/src/CM_WarehouseWriteOffLookup.cpp';
const movementAuditHeaderPath = 'firmware/esp32/src/CM_WarehouseMovementIntegrityAudit.h';
const movementAuditPath = 'firmware/esp32/src/CM_WarehouseMovementIntegrityAudit.cpp';
const controllerPath = 'firmware/esp32/web/shared/writeoff-spool-suggestion.js';

const recovery = read(recoveryPath);
const store = read(storePath);
const storeHeader = read(storeHeaderPath);
const writeoff = read(writeoffPath);
const writeoffStore = read(writeoffStorePath);
const writeoffLookup = read(writeoffLookupPath);
const movementAuditHeader = read(movementAuditHeaderPath);
const movementAudit = read(movementAuditPath);
const controller = read(controllerPath);

for (const text of [
  'm_ready = ensureDirectories()',
  'if (m_ready) m_ready = recoverSpoolFileSwap()',
  'if (m_ready) m_ready = recoverPendingWriteOff()',
  'return m_ready;'
]) {
  requireText(storePath, store, text, 'startup fail-closed recovery ordering missing: ' + text);
}

for (const text of [
  'WarehouseWriteOffStockMode::Unallocated',
  'No stock mutation ever occurs for UNALLOCATED',
  '"ABORTED"'
]) {
  requireText(recoveryPath, recovery, text, 'UNALLOCATED reboot recovery guard missing: ' + text);
}

for (const text of [
  'currentWeight == pending.weightBeforeGrams',
  'currentWeight == pending.weightAfterGrams',
  '"ABORTED"',
  '"CONFIRMED"',
  'return false;'
]) {
  requireText(recoveryPath, recovery, text, 'spool reboot recovery proof missing: ' + text);
}

for (const text of [
  '"/api/warehouse/write-offs", HTTP_POST',
  'm_server.send(410',
  'legacy_writeoff_post_disabled',
  'write_performed',
  'replacement',
  '/api/material-requests/warehouse',
  '"/api/warehouse/write-offs", HTTP_GET',
  'handleListWriteOffs()'
]) {
  requireText(writeoffPath, writeoff, text, 'legacy POST deprecation contract missing: ' + text);
}
for (const forbidden of [
  'handleConfirmWriteOff()',
  'confirmKgFirstWriteOff(operation, result)',
  'confirmSpoolWriteOff(operation, result)',
  'source_run_already_written_off'
]) {
  requireAbsent(writeoffPath, writeoff, forbidden,
    'retired public mutation implementation must stay absent: ' + forbidden);
}

// The old direct Store operations remain implemented for deterministic legacy
// compatibility/recovery, but must not be callable through the public Store API.
const privateMarker = '\nprivate:';
const privatePosition = storeHeader.indexOf(privateMarker);
if (privatePosition < 0) {
  failures.push(storeHeaderPath + ': WarehouseStore private boundary missing');
} else {
  const publicSurface = storeHeader.slice(0, privatePosition);
  const privateSurface = storeHeader.slice(privatePosition);
  for (const method of ['confirmSpoolWriteOff(', 'confirmKgFirstWriteOff(']) {
    if (publicSurface.includes(method)) {
      failures.push(storeHeaderPath + ': legacy direct writeoff method leaked into public API: ' + method);
    }
    if (!privateSurface.includes(method)) {
      failures.push(storeHeaderPath + ': retained private legacy recovery method missing: ' + method);
    }
  }
  for (const managed of [
    'prepareManagedRunWireWriteOff(',
    'applyManagedRunWireSpoolWeight(',
    'confirmManagedRunWireWriteOff('
  ]) {
    if (!publicSurface.includes(managed)) {
      failures.push(storeHeaderPath + ': managed RUN_WIRE API must remain public: ' + managed);
    }
  }
}

for (const source of [
  [storeHeaderPath, storeHeader],
  [writeoffLookupPath, writeoffLookup]
]) {
  requireAbsent(source[0], source[1], 'confirmedWriteOffForSourceSession',
    'obsolete session-only duplicate lookup must stay removed');
}
for (const text of [
  'confirmedWriteOffForSourceRun(uint32_t sourceSessionId',
  'sourceRunId == 0UL',
  'WarehouseMovementIntegrityAudit::checkSourceRun('
]) {
  requireText(writeoffLookupPath, writeoffLookup, text,
    'exact-run duplicate lookup contract missing: ' + text);
}
requireText(storeHeaderPath, storeHeader,
  'confirmedWriteOffForSourceRun(uint32_t sourceSessionId,uint32_t sourceRunId,bool& found) const;',
  'public duplicate lookup must require source_session_id + source_run_id');

for (const text of [
  'static bool checkSourceRun(fs::FS& storage,',
  'uint32_t sourceSessionId,',
  'uint32_t sourceRunId,',
  'bool& confirmed);'
]) {
  requireText(movementAuditHeaderPath, movementAuditHeader, text,
    'movement audit exact-run API missing: ' + text);
}
for (const text of [
  'record.status == "CONFIRMED"',
  'record.sourceSessionId == sourceSessionId',
  'record.sourceRunId == sourceRunId',
  '*sourceRunConfirmed = true;',
  'if (!confirmedProvenanceUnique(storage, Path)) return false;'
]) {
  requireText(movementAuditPath, movementAudit, text,
    'single-pass exact-run resolution/fail-closed audit missing: ' + text);
}
for (const forbidden of [
  'm_storage.open(MovementsPath, FILE_READ)',
  'readStringUntil',
  'findUnsigned(line, "source_session_id"',
  'findUnsigned(line, "source_run_id"'
]) {
  requireAbsent(writeoffLookupPath, writeoffLookup, forbidden,
    'write-off lookup must not restore a post-audit full-file scan: ' + forbidden);
}

for (const text of [
  'WindingSessionCompletionAudit::check',
  'confirmedWriteOffForSourceRun(operation.sourceSessionId',
  'alreadyConfirmed',
  'if (!ready()'
]) {
  requireText(writeoffStorePath, writeoffStore, text, 'store-level fault guard missing: ' + text);
}

for (const text of [
  "if(!response.ok){const error=new Error(payload.error||('http_'+response.status))",
  "const data=await jsonFetch('/api/material-requests/warehouse'",
  "if(!validId(data.movement_id)||String(data.spool_id)!==String(activeSpool.spool_id))throw new Error('run_wire_response_identity_mismatch')",
  "setResult('RUN_WIRE: списано '+kgFromGrams(quantity.grams)",
  'await loadHistory(0,true);',
  'await prepareNextRun();',
  "catch(error){\n        setResult('Ошибка: '+error.message,'bad');"
]) {
  requireText(controllerPath, controller, text, 'manual atomic RUN_WIRE UI fault semantics missing: ' + text);
}
requireAbsent(controllerPath, controller,
  "jsonFetch('/api/warehouse/write-offs',{method:'POST'",
  'production operator UI must not return to legacy/direct write-off POST');

for (const forbidden of [
  'automaticWriteOff(',
  'autoWriteOff(',
  'writeOffOnRunCompleted(',
  'autoResume(',
  'automaticPhysicalStart('
]) {
  if (recovery.includes(forbidden) || store.includes(forbidden) ||
      writeoff.includes(forbidden) || writeoffStore.includes(forbidden) ||
      controller.includes(forbidden)) {
    failures.push('fault-path code introduced forbidden automatic action: ' + forbidden);
  }
}

if (failures.length) {
  console.error(failures.join('\n'));
  process.exit(1);
}
console.log('Write-off fault contracts OK: public legacy POST is 410-disabled, legacy direct Store methods are private-only, managed RUN_WIRE APIs stay public, deterministic recovery/history remain intact, and no automatic deduction exists.');
