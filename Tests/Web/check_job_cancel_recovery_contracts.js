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

const espTransportPath = 'firmware/esp32/src/CM_UartEventReceiver.cpp';
const espMainPath = 'firmware/esp32/src/main.cpp';
const remoteCancelStatePath = 'firmware/esp32/src/CM_JobStateRemoteCancel.cpp';
const dismissStatePath = 'firmware/esp32/src/CM_JobStateDismiss.cpp';
const recoveryPath = 'firmware/esp32/src/CM_JobRecovery.cpp';
const journalSnapshotPath = 'firmware/esp32/src/CM_WindingJournalSnapshotContext.cpp';
const arduinoMainPath = 'firmware/arduino/src/main.cpp';
const arduinoTransportPath = 'Arduino/CM_UartEventTransport.cpp';

const espTransport = read(espTransportPath);
const espMain = read(espMainPath);
const remoteCancelState = read(remoteCancelStatePath);
const dismissState = read(dismissStatePath);
const recovery = read(recoveryPath);
const journalSnapshot = read(journalSnapshotPath);
const arduinoMain = read(arduinoMainPath);
const arduinoTransport = read(arduinoTransportPath);

// If a JOB frame may already have reached Arduino, cancelling on ESP32 must
// switch from local cancellation to the idempotent remote JOB_CANCEL handshake.
for (const text of [
  'const bool mayHaveReachedArduino = m_waitingJobAck || sendAttempts != 0U;',
  'if (!mayHaveReachedArduino)',
  'm_cancelJobId = jobId;',
  'm_hasPendingCancel = true;',
  'CMP1|JOB_CANCEL|%lu'
]) {
  requireText(espTransportPath, espTransport, text,
    'lost-ACK ghost-job cancellation contract missing: ' + text);
}

// A physical Arduino fallback publishes a CRC-protected job_id=0 ALL_CLEAR.
// ESP32 must correlate that proof to the current/recovered job and route it
// through the normal cancellation event, never as winding completion evidence.
for (const text of [
  'if (jobId == 0UL)',
  'strcmp(detail, "ALL_CLEAR") != 0',
  'm_hasPendingCancel\n            ? m_cancelJobId',
  'publishJobCancel(JobCancelResult::Cancelled,',
  '"ALL_CLEAR"'
]) {
  requireText(espTransportPath, espTransport, text,
    'ESP32 ALL_CLEAR recovery contract missing: ' + text);
}

// A positive cancel acknowledgement must close persisted state only through the
// dedicated no-run transition, then re-evaluate recovery before a new job can be
// permitted. Immutable snapshot/history is intentionally not deleted here.
for (const text of [
  'void processJobCancel()',
  '!jobStates.closeAfterRemoteCancel(sessionId, millis())',
  'restoreLatestJobState();',
  'if (!jobStateStoreReady || manualReviewRequired())',
  'recoveryInfo.mayCreateNewJob = false;'
]) {
  requireText(espMainPath, espMain, text,
    'ESP32 persisted cancel recovery contract missing: ' + text);
}
for (const text of [
  'state.executionState == JobExecutionState::WaitingDelivery ||',
  'state.executionState == JobExecutionState::WaitingPhysicalStart;',
  'state.lastRunId != 0UL || state.completedRuns != 0U',
  'state.deliveryState = JobDeliveryState::Cancelled;',
  'state.executionState = JobExecutionState::ClosedAfterReview;'
]) {
  requireText(remoteCancelStatePath, remoteCancelState, text,
    'persisted remote-cancel no-run safety contract missing: ' + text);
}

// A late duplicate CANCELLED/ALL_CLEAR after an already completed or explicitly
// closed job is stale transport evidence. It must be a no-op, not a storage
// failure, and must never rewrite completed run evidence as cancellation.
for (const text of [
  'state.executionState == JobExecutionState::ProgramCompleted ||',
  'state.executionState == JobExecutionState::ClosedAfterReview)',
  'return true;'
]) {
  requireText(remoteCancelStatePath, remoteCancelState, text,
    'stale cancel terminal-state no-op contract missing: ' + text);
}

// TIMED_OUT is ambiguous because Arduino may have accepted the JOB while every
// ACK was lost. Ordinary inactive dismissal must never treat timeout as terminal;
// only the explicit manual-review closure path may resolve it.
requireText(recoveryPath, recovery,
  'if (state.deliveryState == JobDeliveryState::TimedOut)',
  'JobRecovery must keep timed-out delivery in manual review');
requireText(dismissStatePath, dismissState,
  'state.deliveryState == JobDeliveryState::Rejected ||\n        state.deliveryState == JobDeliveryState::Cancelled;',
  'ordinary dismiss must be limited to proven terminal delivery states');
requireAbsent(dismissStatePath, dismissState,
  'state.deliveryState == JobDeliveryState::TimedOut ||',
  'TIMED_OUT must not be dismissible without manual review');

// Immutable repeat_target is authoritative before a winding event is appended.
// The journal wrapper must reject a new RUN_STARTED once the persisted job has
// completed its target, and reject any RUN_COMPLETED whose cumulative count is
// already beyond the immutable target. Use the small state file, not another
// full NDJSON scan, to preserve growing-log performance.
for (const text of [
  '#include "CM_JobStateStore.h"',
  'JobStateStore states(m_fileSystem);',
  'runtime.jobId != snapshot.jobId || runtime.sessionId != snapshot.sessionId',
  'runtime.executionState == JobExecutionState::ProgramCompleted ||',
  'runtime.completedRuns >= snapshot.repeatTarget',
  'event.completedRuns > snapshot.repeatTarget'
]) {
  requireText(journalSnapshotPath, journalSnapshot, text,
    'immutable repeat-target journal guard missing: ' + text);
}
requireAbsent(journalSnapshotPath, journalSnapshot,
  'loadSessionState(event.sessionId',
  'repeat-target guard must not add another full journal state scan');

// Arduino remote cancel remains safe, exact and idempotent for a job that is
// already absent. ALREADY_CLEAR is success so retries/reboots cannot strand ESP32.
for (const text of [
  'void processRemoteCancels()',
  'espTransport.takeRemoteCancel(jobId)',
  'detail = "ALREADY_CLEAR";',
  'espTransport.sendJobCancelResult(jobId, cancelled, detail);'
]) {
  requireText(arduinoMainPath, arduinoMain, text,
    'Arduino idempotent remote cancel contract missing: ' + text);
}

// The local emergency sequence can clear only a no-run READY remote job. It
// must reject an active physical run and emit ALL_CLEAR only after Arduino has
// proved that it holds no remote job.
for (const text of [
  "static const char Sequence[] = {'D', '*', '#', 'D'};",
  'machine.state() == CM::MachineState::Ready &&',
  'active.currentRunId == 0UL && active.completedRuns == 0U;',
  'if (!safeRemoteReady || !machine.cancel())',
  'CM_JOB EMERGENCY_CLEAR result=REJECTED_ACTIVE_RUN',
  'espTransport.sendJobClear();'
]) {
  requireText(arduinoMainPath, arduinoMain, text,
    'Arduino physical ALL_CLEAR safety contract missing: ' + text);
}

requireText(arduinoTransportPath, arduinoTransport,
  'CMP1|JOB_CANCEL_ACK|0|CANCELLED|ALL_CLEAR|C',
  'Arduino CRC-capable ALL_CLEAR frame missing');

if (failures.length) {
  console.error(failures.join('\n'));
  process.exit(1);
}

console.log('JOB lifecycle contracts OK: lost-ACK remote cancel, idempotent ALREADY_CLEAR, safe physical ALL_CLEAR, active-run rejection, persisted no-run closure, stale terminal cancel no-op, timeout manual-review isolation, immutable repeat-target journal guard, and recovery re-evaluation.');
