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

const recoveryPath = 'firmware/esp32/src/CM_WarehouseWriteOffRecovery.cpp';
const storePath = 'firmware/esp32/src/CM_WarehouseStore.cpp';
const writeoffPath = 'firmware/esp32/src/CM_WarehouseWriteOffWeb.cpp';
const writeoffStorePath = 'firmware/esp32/src/CM_WarehouseWriteOff.cpp';
const controllerPath = 'firmware/esp32/web/shared/writeoff-spool-suggestion.js';

const recovery = read(recoveryPath);
const store = read(storePath);
const writeoff = read(writeoffPath);
const writeoffStore = read(writeoffStorePath);
const controller = read(controllerPath);

// Startup must resolve an interrupted stock-file swap before examining a pending
// write-off, and readiness must remain false if either recovery stage fails.
for (const text of [
  'm_ready = ensureDirectories()',
  'if (m_ready) m_ready = recoverSpoolFileSwap()',
  'if (m_ready) m_ready = recoverPendingWriteOff()',
  'return m_ready;'
]) {
  requireText(storePath, store, text, 'startup fail-closed recovery ordering missing: ' + text);
}

// A reboot after UNALLOCATED PENDING can never invent a confirmed deduction:
// there was no durable stock mutation that could prove confirmation.
for (const text of [
  'WarehouseWriteOffStockMode::Unallocated',
  'No stock mutation ever occurs for UNALLOCATED',
  '"ABORTED"'
]) {
  requireText(recoveryPath, recovery, text, 'UNALLOCATED reboot recovery guard missing: ' + text);
}

// A spool-backed interrupted transaction is resolved only from durable spool
// state. BEFORE means no mutation -> ABORTED; AFTER means mutation happened -> CONFIRMED;
// any third weight is ambiguous and must fail recovery rather than guessing.
for (const text of [
  'currentWeight == pending.weightBeforeGrams',
  'currentWeight == pending.weightAfterGrams',
  '"ABORTED"',
  '"CONFIRMED"',
  'return false;'
]) {
  requireText(recoveryPath, recovery, text, 'spool reboot recovery proof missing: ' + text);
}

// HTTP writes must fail closed when warehouse/storage is unavailable, when the
// repair is closed, when RUN_COMPLETED cannot be proven, or when the exact run
// already has a confirmed write-off.
for (const text of [
  '"warehouse_unavailable"',
  '"repair_closed"',
  '"winding_history_unavailable"',
  '"winding_history_integrity_failed"',
  '"source_run_not_completed"',
  'confirmedWriteOffForSourceRun(sourceSessionId, sourceRunId, alreadyConfirmed)',
  '"source_run_already_written_off"',
  '"write_performed\\":false'
]) {
  requireText(writeoffPath, writeoff, text, 'write-off HTTP fault guard missing: ' + text);
}

// Store-level duplicate/completion protection must remain authoritative even if
// a future caller bypasses the web handler.
for (const text of [
  'WindingSessionCompletionAudit::check',
  'confirmedWriteOffForSourceRun(operation.sourceSessionId',
  'alreadyConfirmed',
  'if (!ready()'
]) {
  requireText(writeoffStorePath, writeoffStore, text, 'store-level fault guard missing: ' + text);
}

// UI must not present a failed transaction as successful and must re-query the
// next uncovered completed run only after a successful confirmed response.
for (const text of [
  "if(!response.ok)",
  "if(payload&&payload.confirmed===true",
  "setTimeout(run,600)",
  "write_performed:false"
]) {
  requireText(controllerPath, controller, text, 'manual UI fault semantics missing: ' + text);
}

// Safety invariants: recovery and fault handling must never create automatic
// physical start, automatic resume, or RUN_COMPLETED-triggered material deduction.
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
console.log('Write-off fault contracts OK: fail-closed storage/repair/run guards, exact-run duplicate protection, deterministic reboot recovery, and no automatic deduction.');
