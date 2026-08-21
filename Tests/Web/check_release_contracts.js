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
const esp32Path = 'firmware/esp32/src/main.cpp';
const writeOffPath = 'firmware/esp32/src/CM_WarehouseWriteOffWeb.cpp';
const restorePath = 'firmware/esp32/src/CM_RemoteBackupWeb.cpp';
const webAuditPath = 'Tests/Web/check_web_assets.js';

const arduino = read(arduinoPath);
const esp32 = read(esp32Path);
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
}

// Recovery remains operator-controlled: no automatic queue/resume/writeoff after reboot.
requireText(esp32Path, esp32,
  '\\"automatic_queue_allowed\\":false,\\"automatic_resume_allowed\\":false,\\"automatic_wire_writeoff_allowed\\":false',
  'fail-closed automatic recovery/writeoff status contract missing');

// Wire writeoff is manual and tied to the exact selected spool + completed source run.
for (const field of ['spool_id', 'source_session_id', 'source_run_id']) {
  requireText(writeOffPath, writeOff, '"' + field + '"',
    'required manual writeoff field ' + field + ' missing');
}
requireText(writeOffPath, writeOff,
  'selection.repairId != repairId || selection.spoolId != spoolId',
  'writeoff no longer verifies exact repair/spool selection');
requireText(writeOffPath, writeOff,
  'WindingSessionCompletionAudit::check(m_store.storage(),',
  'writeoff no longer verifies the source run completion');
requireText(writeOffPath, writeOff,
  'confirmedWriteOffForSourceRun(sourceSessionId,',
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

console.log('Release safety contracts OK: physical START, Arduino SSR authority, Hall calibration permit, no auto-resume/writeoff, exact manual writeoff linkage, transactional restore lock, and motor-import audits.');
