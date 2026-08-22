const fs = require('fs');
const path = require('path');

const root = path.resolve(__dirname, '../..');
const storePath = 'firmware/esp32/src/CM_JobStateStore.cpp';
const recoveryPath = 'firmware/esp32/src/CM_JobStateLateRunRecovery.cpp';
const mainPath = 'firmware/esp32/src/main.cpp';
const store = fs.readFileSync(path.join(root, storePath), 'utf8');
const recovery = fs.readFileSync(path.join(root, recoveryPath), 'utf8');
const main = fs.readFileSync(path.join(root, mainPath), 'utf8');
const failures = [];

function requireText(source, file, text, description) {
  if (!source.includes(text)) failures.push(`${file}: ${description}: ${text}`);
}

// A lost JOB_ACK is ambiguous. Normal timeout handling must remain fail-closed;
// only later physical RUN_STARTED evidence may promote the exact persisted state.
requireText(recovery, recoveryPath,
  'state.deliveryState != JobDeliveryState::TimedOut',
  'late-start helper no longer restricts reconciliation to TIMED_OUT');
requireText(recovery, recoveryPath,
  'state.executionState != JobExecutionState::WaitingDelivery',
  'late-start helper no longer requires pre-run WAITING_DELIVERY');
requireText(recovery, recoveryPath,
  'state.lastRunId != 0UL || state.completedRuns != 0U',
  'late-start helper no longer rejects prior run evidence');
requireText(recovery, recoveryPath,
  'state.deliveryState = JobDeliveryState::Accepted;',
  'late physical evidence is not persisted as accepted delivery');
requireText(recovery, recoveryPath,
  'state.executionState = JobExecutionState::Running;',
  'late physical evidence is not persisted as RUNNING');

// Production RUN_STARTED persistence must actually reach the helper. Before this
// contract, the helper existed but updateExecution rejected WAITING_DELIVERY ->
// RUNNING first, causing STATE_WRITE_FAILED after a valid late RUN_STARTED.
requireText(store, storePath,
  'executionState == JobExecutionState::Running &&\n        state.deliveryState == JobDeliveryState::TimedOut &&',
  'updateExecution does not detect the narrow late-start timeout state');
requireText(store, storePath,
  'if (runId == 0UL || completedRuns != 0U) return false;',
  'late RUN_STARTED reconciliation no longer requires zero completion evidence');
requireText(store, storePath,
  'return confirmStartedAfterDeliveryTimeout(sessionId, runId, nowMs);',
  'updateExecution no longer routes late RUN_STARTED to the dedicated helper');
requireText(main, mainPath,
  'return jobStates.updateExecution(event.sessionId,\n                                         CM::JobExecutionState::Running,',
  'production RUN_STARTED no longer flows through JobStateStore::updateExecution');

// Do not "fix" timeout by permitting new work or auto-resume.
if (recovery.includes('mayCreateNewJob = true') || recovery.includes('queueJob(') || recovery.includes('start')) {
  // The function name contains Started; inspect only dangerous call spellings.
  if (recovery.includes('queueJob(') || recovery.includes('startOrResume(') || recovery.includes('physicalStart(')) {
    failures.push(`${recoveryPath}: late-start reconciliation must never queue or physically start work`);
  }
}

if (failures.length) {
  console.error(failures.join('\n'));
  process.exit(1);
}

console.log('Late RUN_STARTED recovery contracts OK: timeout remains fail-closed and only exact physical-run evidence reconciles delivery state.');
