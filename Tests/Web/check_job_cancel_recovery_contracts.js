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
const espTransportHeaderPath = 'firmware/esp32/src/CM_UartEventReceiver.h';
const espMainPath = 'firmware/esp32/src/main.cpp';
const remoteCancelStatePath = 'firmware/esp32/src/CM_JobStateRemoteCancel.cpp';
const lateRunRecoveryPath = 'firmware/esp32/src/CM_JobStateLateRunRecovery.cpp';
const dismissStatePath = 'firmware/esp32/src/CM_JobStateDismiss.cpp';
const recoveryPath = 'firmware/esp32/src/CM_JobRecovery.cpp';
const journalSnapshotPath = 'firmware/esp32/src/CM_WindingJournalSnapshotContext.cpp';
const sessionAuditPath = 'firmware/esp32/src/CM_WindingSessionPersistenceIntegrityAudit.cpp';
const arduinoMainPath = 'firmware/arduino/src/main.cpp';
const arduinoTransportPath = 'Arduino/CM_UartEventTransport.cpp';

const espTransport = read(espTransportPath);
const espTransportHeader = read(espTransportHeaderPath);
const espMain = read(espMainPath);
const remoteCancelState = read(remoteCancelStatePath);
const lateRunRecovery = read(lateRunRecoveryPath);
const dismissState = read(dismissStatePath);
const recovery = read(recoveryPath);
const journalSnapshot = read(journalSnapshotPath);
const sessionAudit = read(sessionAuditPath);
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

// Control results must be persisted before a following physical RUN is parsed.
// This covers both UART replies and timeouts published synchronously by update().
for (const text of [
  'if (m_hasJobDelivery || m_hasJobCancel) return false;',
  'bool orderingBarrier = false;',
  'processJobAck(m_line);\n                    orderingBarrier = true;',
  'processCancelAck(m_line);\n                    orderingBarrier = true;',
  'if (orderingBarrier) return false;'
]) {
  requireText(espTransportPath, espTransport, text,
    'JOB/CANCEL-before-RUN ordering barrier missing: ' + text);
}
for (const text of [
  'while (receiver.poll(event)) handleEvent(event);',
  'processJobDelivery();',
  'processJobCancel();'
]) {
  requireText(espMainPath, espMain, text,
    'main loop persistence ordering contract missing: ' + text);
}

// TIMED_OUT delivery is ambiguous, but a CRC-valid RUN_STARTED for the exact
// immutable session proves Arduino accepted and physically started it. Reconcile
// only TIMED_OUT + WAITING_DELIVERY + zero run evidence. The event stays
// journal-first/retry-safe: Saved/Duplicate then atomic state recovery.
for (const text of [
  'state.deliveryState != JobDeliveryState::TimedOut',
  'state.executionState != JobExecutionState::WaitingDelivery',
  'state.lastRunId != 0UL || state.completedRuns != 0U',
  'state.deliveryState = JobDeliveryState::Accepted;',
  'state.executionState = JobExecutionState::Running;',
  'state.lastRunId = runId;',
  'return writeAtomic(state);'
]) {
  requireText(lateRunRecoveryPath, lateRunRecovery, text,
    'narrow late-RUN timeout recovery contract missing: ' + text);
}
for (const text of [
  'const bool recoverTimedOutStart =',
  'runtime.deliveryState == JobDeliveryState::TimedOut;',
  'const JournalSaveResult result = save(event, context);',
  'result == JournalSaveResult::Duplicate',
  'states.confirmStartedAfterDeliveryTimeout(event.sessionId,',
  'return JournalSaveResult::StorageUnavailable;'
]) {
  requireText(journalSnapshotPath, journalSnapshot, text,
    'journal-first timeout RUN reconciliation missing: ' + text);
}

// Once physical RUN evidence is committed, RAM must stop reporting the old
// timeout and the recovery-only zero-id ALL_CLEAR identity must be disarmed.
for (const text of [
  'lastJobResult = CM::JobDeliveryResult::Accepted;',
  'receiver.clearRecoveryJobId(activeJobId);'
]) {
  requireText(espMainPath, espMain, text,
    'runtime normalization after RUN evidence missing: ' + text);
}
for (const text of [
  'void clearRecoveryJobId(uint32_t jobId)',
  'm_hasRecoveryJobId && m_recoveryJobId == jobId',
  'm_recoveryJobId = 0UL;',
  'm_hasRecoveryJobId = false;'
]) {
  requireText(espTransportHeaderPath, espTransportHeader, text,
    'recovery identity clear contract missing: ' + text);
}

// A physical Arduino fallback may publish a CRC-protected job_id=0 ALL_CLEAR.
// Zero has no job identity, so it may resolve only an explicit pending cancel or
// an identity explicitly remembered during persisted reboot recovery. queueJob()
// clears that fallback before any fresh JOB is sent.
for (const text of [
  'm_recoveryJobId(0UL), m_hasRecoveryJobId(false)',
  'm_recoveryJobId = jobId;\n    m_hasRecoveryJobId = true;',
  'if (jobId == 0UL)',
  'strcmp(detail, "ALL_CLEAR") != 0',
  'const uint32_t clearedJobId = m_hasPendingCancel',
  '? m_cancelJobId',
  ': (m_hasRecoveryJobId ? m_recoveryJobId : 0UL);',
  'publishJobCancel(JobCancelResult::Cancelled,',
  '"ALL_CLEAR"'
]) {
  requireText(espTransportPath, espTransport, text,
    'ESP32 recovery-only ALL_CLEAR correlation contract missing: ' + text);
}
for (const forbidden of [
  'm_hasPendingJob ? m_pendingJob.jobId : m_lastQueuedJobId',
  'm_hasLastQueuedJobId ? m_lastQueuedJobId : 0UL'
]) {
  requireAbsent(espTransportPath, espTransport, forbidden,
    'zero-id ALL_CLEAR must never infer identity from a fresh/last queued JOB: ' + forbidden);
}
requireText(espMainPath, espMain,
  'receiver.rememberJobId(activeJobId);',
  'persisted recovery must explicitly arm the zero-id ALL_CLEAR fallback identity');

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
  'if (!waitingOnly || state.lastRunId != 0UL || state.completedRuns != 0U)\n        return true;',
  'state.deliveryState = JobDeliveryState::Cancelled;',
  'state.executionState = JobExecutionState::ClosedAfterReview;'
]) {
  requireText(remoteCancelStatePath, remoteCancelState, text,
    'persisted remote-cancel safety/no-op contract missing: ' + text);
}

// Late CANCELLED/ALL_CLEAR for completed/closed or run/fault evidence is stale
// transport evidence. It must be a no-op, not a storage failure, and must never
// rewrite physical run evidence as cancellation.
for (const text of [
  'state.executionState == JobExecutionState::ProgramCompleted ||',
  'state.executionState == JobExecutionState::ClosedAfterReview)',
  'return true;'
]) {
  requireText(remoteCancelStatePath, remoteCancelState, text,
    'stale/run-evidence cancel no-op contract missing: ' + text);
}

// TIMED_OUT remains manual-review state when no stronger RUN evidence arrives.
requireText(recoveryPath, recovery,
  'if (state.deliveryState == JobDeliveryState::TimedOut)',
  'JobRecovery must keep unresolved timed-out delivery in manual review');
requireText(dismissStatePath, dismissState,
  'state.deliveryState == JobDeliveryState::Rejected ||\n        state.deliveryState == JobDeliveryState::Cancelled;',
  'ordinary dismiss must be limited to proven terminal delivery states');
requireAbsent(dismissStatePath, dismissState,
  'state.deliveryState == JobDeliveryState::TimedOut ||',
  'unresolved TIMED_OUT must not be dismissible without manual review');

// Immutable repeat_target remains authoritative before accepting run evidence.
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

for (const text of [
  'bool stateConsistentWithSnapshot(const JobRuntimeState& state,',
  'state.completedRuns > snapshot.repeatTarget',
  'state.executionState == JobExecutionState::ProgramCompleted',
  'state.completedRuns == snapshot.repeatTarget',
  'state.completedRuns < snapshot.repeatTarget',
  '!stateConsistentWithSnapshot(state, snapshot)'
]) {
  requireText(sessionAuditPath, sessionAudit, text,
    'repeat-target session integrity guard missing: ' + text);
}
requireAbsent(sessionAuditPath, sessionAudit,
  'WindingJournalQuery',
  'session repeat-target integrity must not add a duplicate journal scan');

// Arduino remote cancel remains exact and idempotent for an already absent job.
for (const text of [
  'void processRemoteCancels()',
  'espTransport.takeRemoteCancel(jobId)',
  'detail = "ALREADY_CLEAR";',
  'espTransport.sendJobCancelResult(jobId, cancelled, detail);'
]) {
  requireText(arduinoMainPath, arduinoMain, text,
    'Arduino idempotent remote cancel contract missing: ' + text);
}

// Physical emergency clear is allowed only for a zero-run READY remote job.
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

console.log('JOB lifecycle contracts OK: control-before-RUN ordering, timeout RUN reconciliation, runtime normalization, lost-ACK remote cancel, recovery-only ALL_CLEAR, run-evidence cancel no-op, timeout manual-review isolation, repeat-target integrity, and Arduino physical clear safety.');
