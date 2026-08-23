const fs = require('fs');
const path = require('path');

const root = path.resolve(__dirname, '../..');
const failures = [];

function read(relative) {
  return fs.readFileSync(path.join(root, relative), 'utf8');
}

function requireText(relative, source, text, description) {
  if (!source.includes(text)) {
    failures.push(relative + ': ' + description);
  }
}

function requireAbsent(relative, source, pattern, description) {
  if (pattern.test(source)) {
    failures.push(relative + ': ' + description);
  }
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
const restorePath = 'firmware/esp32/src/CM_RemoteBackupWeb.cpp';
const webAuditPath = 'Tests/Web/check_web_assets.js';

const arduino = read(arduinoPath);
const recovery = read(recoveryPath);
const writeOff = read(writeOffPath);
const restore = read(restorePath);
const webAudit = read(webAuditPath);

// Physical START authority belongs to Arduino and remains a debounced physical input.
requireText(arduinoPath, arduino,
  'if (startButton.pollPressed(nowMs))',
  'physical START button polling contract missing');
requireText(arduinoPath, arduino,
  'event.action = CM::InputAction::StartOrResume;',
  'physical START no longer maps to StartOrResume');
requireText(arduinoPath, arduino,
  'machine.resetToHome();',
  'boot must return the winding state machine to HOME');

// SSR authority belongs to Arduino. ESP32/Web must not gain a direct SSR driver path.
// Hall calibration may grant the Arduino-local motor permit, but physical START and
// Arduino state remain the only production authority for energizing the SSR.
requireText(arduinoPath, arduino,
  'CM::SsrController ssr(CM::Pins::Ssr, true);',
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

// Recovery remains operator-controlled after reboot. Assert the actual state-machine
// policy rather than a presentation-only JSON string that may be refactored away.
for (const text of [
  'mayAutoQueue(false)',
  'mayAutoResume(false)',
  'recovery.mayAutoQueue = false;',
  'recovery.mayAutoResume = false;',
  'if (state.deliveryState == JobDeliveryState::TimedOut)',
  'recovery.disposition = JobRecoveryDisposition::ManualReviewRequired;',
  'recovery.mayCreateNewJob = false;'
]) {
  requireText(recoveryPath, recovery, text,
    'fail-closed reboot recovery policy missing: ' + text);
}

// Wire writeoff is always manual and tied to exact completed source-run provenance.
// Both current KG_FIRST and legacy weight-before/after paths require the exact immutable
// selected spool; historical unallocated data remains read/recovery compatibility only.
for (const field of ['spool_id', 'source_session_id', 'source_run_id', 'writeoff_mode']) {
  requireText(writeOffPath, writeOff, '"' + field + '"',
    'required manual writeoff field ' + field + ' missing');
}
requireText(writeOffPath, writeOff,
  'const bool kgFirst = hasMode && m_server.arg("writeoff_mode") == "KG_FIRST";',
  'explicit kg-first manual writeoff mode missing');
requireText(writeOffPath, writeOff,
  'spool_id_required_for_kg_first',
  'kg-first writeoff no longer requires an exact spool_id');
requireText(writeOffPath, writeOff,
  'selection.repairId != repairId || selection.spoolId != spoolId',
  'writeoff no longer preserves repair linkage and mandatory exact spool match');
requireText(writeOffPath, writeOff,
  'WindingSessionCompletionAudit::check(m_store.storage(), sourceSessionId, sourceRunId)',
  'writeoff no longer verifies the exact source run completion');
requireText(writeOffPath, writeOff,
  'confirmedWriteOffForSourceRun(sourceSessionId, sourceRunId, alreadyConfirmed)',
  'duplicate source-run writeoff guard missing');
requireText(writeOffPath, writeOff,
  'operation.sourceSessionId = sourceSessionId;',
  'writeoff operation lost source_session_id binding');
requireText(writeOffPath, writeOff,
  'operation.sourceRunId = sourceRunId;',
  'writeoff operation lost source_run_id binding');

// Restore apply remains explicit, transactional and fail-closed across reboot evidence.
requireText(restorePath, restore,
  'm_server.arg("confirmed") != F("APPLY")',
  'explicit APPLY confirmation contract missing');
requireText(restorePath, restore,
  'm_storage.exists(ApplyJournalPath) ||',
  'persisted apply journal lock missing');
requireText(restorePath, restore,
  'm_storage.exists(ApplyResultMarkerPath)',
  'persisted apply result lock missing');
requireText(restorePath, restore,
  'const bool runtimeApplyActive =',
  'runtime/stale apply distinction missing');
requireText(restorePath, restore,
  'WAITING_RESTORE_CLEANUP',
  'scheduled backup stale-restore wait state missing');
requireText(restorePath, restore,
  'auto_resume=0',
  'restore result must explicitly prohibit auto-resume');

// Keep executable desktop + mobile motor-import validation in the standard web audit.
requireText(webAuditPath, webAudit,
  "auditMotorImport('desktop/motor-import.html');",
  'desktop motor-import executable audit missing');
requireText(webAuditPath, webAudit,
  "auditMotorImport('mobile/motor-import.html');",
  'mobile motor-import executable audit missing');

if (failures.length) {
  console.error(failures.join('\n'));
  process.exit(1);
}

console.log('Release safety contracts OK: physical START, Arduino SSR authority, Hall calibration permit, fail-closed reboot recovery, no automatic start/resume/writeoff action, mandatory exact-spool and exact source-run manual writeoff provenance, transactional restore lock, and motor-import audits.');
