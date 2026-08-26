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

function requireAbsent(relative, source, pattern, description) {
  if (pattern.test(source)) failures.push(relative + ': ' + description);
}

function walkText(directory, extensions) {
  const files = [];
  for (const entry of fs.readdirSync(directory, {withFileTypes: true})) {
    const full = path.join(directory, entry.name);
    if (entry.isDirectory()) files.push(...walkText(full, extensions));
    else if (entry.isFile() && extensions.has(path.extname(entry.name))) files.push(full);
  }
  return files;
}

const arduinoPath = 'firmware/arduino/src/main.cpp';
const recoveryPath = 'firmware/esp32/src/CM_JobRecovery.cpp';
const writeOffPath = 'firmware/esp32/src/CM_WarehouseWriteOffWeb.cpp';
const writeOffStorePath = 'firmware/esp32/src/CM_WarehouseWriteOff.cpp';
const runWirePath = 'firmware/esp32/src/CM_RunWireIssueCoordinator.cpp';
const restorePath = 'firmware/esp32/src/CM_RemoteBackupWeb.cpp';
const webAuditPath = 'Tests/Web/check_web_assets.js';

const arduino = read(arduinoPath);
const recovery = read(recoveryPath);
const writeOff = read(writeOffPath);
const writeOffStore = read(writeOffStorePath);
const runWire = read(runWirePath);
const restore = read(restorePath);
const webAudit = read(webAuditPath);

requireText(arduinoPath, arduino, 'if (startButton.pollPressed(nowMs))',
  'physical START button polling contract missing');
requireText(arduinoPath, arduino, 'event.action = CM::InputAction::StartOrResume;',
  'physical START no longer maps to StartOrResume');
requireText(arduinoPath, arduino, 'machine.resetToHome();',
  'boot must return the winding state machine to HOME');

requireText(arduinoPath, arduino, 'CM::SsrController ssr(CM::Pins::Ssr, true);',
  'Arduino SSR controller ownership missing');
requireText(arduinoPath, arduino,
  'ssr.update(machine.state(),\n               simulationMode(),\n               hallCalibration.motorPermit());',
  'SSR output is no longer derived from Arduino machine state plus calibration permit');
const esp32SourceRoot = path.join(root, 'firmware/esp32/src');
for (const file of walkText(esp32SourceRoot, new Set(['.cpp', '.h']))) {
  const relative = path.relative(root, file).split(path.sep).join('/');
  const source = fs.readFileSync(file, 'utf8');
  requireAbsent(relative, source, /\bSsrController\b|\bPins::Ssr\b/,
    'direct SSR authority detected on ESP32');
  requireAbsent(relative, source,
    /\b(?:automaticWriteOff|autoWriteOff|writeOffOnRunCompleted|autoResume|automaticPhysicalStart)\s*\(/,
    'forbidden automatic start/resume/writeoff action detected on ESP32');
}

for (const text of [
  'mayAutoQueue(false)',
  'mayAutoResume(false)',
  'recovery.mayAutoQueue = false;',
  'recovery.mayAutoResume = false;',
  'if (state.deliveryState == JobDeliveryState::TimedOut)',
  'recovery.disposition = JobRecoveryDisposition::ManualReviewRequired;',
  'recovery.mayCreateNewJob = false;'
]) {
  requireText(recoveryPath, recovery, text, 'fail-closed reboot recovery policy missing: ' + text);
}

// Only atomic Material Request RUN_WIRE is publicly mutable. The old warehouse
// POST is explicitly gone (410); GET history remains. Exact spool/session/run
// checks are still mandatory in the atomic coordinator and retained low-level
// store/recovery implementation.
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
  requireText(writeOffPath, writeOff, text, 'legacy writeoff deprecation contract missing: ' + text);
}
if (writeOff.includes('handleConfirmWriteOff')) {
  failures.push(writeOffPath + ': retired legacy mutation handler returned');
}
for (const text of [
  'JobSpoolSelectionStore::loadReadOnly',
  'selection.spoolId != spoolId',
  'WindingSessionCompletionAudit::check',
  'm_warehouse.confirmedWriteOffForSourceRun',
  'm_pending.save(pending)'
]) {
  requireText(runWirePath, runWire, text, 'atomic exact-spool/source-run writeoff guard missing: ' + text);
}
for (const text of [
  'WindingSessionCompletionAudit::check',
  'confirmedWriteOffForSourceRun(operation.sourceSessionId',
  'operation.spoolId == 0UL || selection.spoolId != operation.spoolId',
  'alreadyConfirmed'
]) {
  requireText(writeOffStorePath, writeOffStore, text, 'retained low-level writeoff safety missing: ' + text);
}

requireText(restorePath, restore, 'm_server.arg("confirmed") != F("APPLY")',
  'explicit APPLY confirmation contract missing');
requireText(restorePath, restore, 'm_storage.exists(ApplyJournalPath) ||',
  'persisted apply journal lock missing');
requireText(restorePath, restore, 'm_storage.exists(ApplyResultMarkerPath)',
  'persisted apply result lock missing');
requireText(restorePath, restore, 'const bool runtimeApplyActive =',
  'runtime/stale apply distinction missing');
requireText(restorePath, restore, 'WAITING_RESTORE_CLEANUP',
  'scheduled backup stale-restore wait state missing');
requireText(restorePath, restore, 'auto_resume=0',
  'restore result must explicitly prohibit auto-resume');

requireText(webAuditPath, webAudit, "auditMotorImport('desktop/motor-import.html');",
  'desktop motor-import executable audit missing');
requireText(webAuditPath, webAudit, "auditMotorImport('mobile/motor-import.html');",
  'mobile motor-import executable audit missing');

if (failures.length) {
  console.error(failures.join('\n'));
  process.exit(1);
}

console.log('Release safety contracts OK: physical START and SSR remain Arduino-owned, reboot is fail-closed, legacy writeoff POST is 410-disabled, atomic RUN_WIRE preserves exact spool/run provenance, no automatic deduction exists, and restore stays transactional.');
