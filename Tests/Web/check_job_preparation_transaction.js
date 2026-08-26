const fs = require('fs');
const path = require('path');

require('./check_linked_job_winding_role.js');
require('./check_winding_job_role_ui.js');

const root = path.resolve(__dirname, '../..');
const failures = [];
function read(relative) { return fs.readFileSync(path.join(root, relative), 'utf8'); }
function requireText(relative, source, text, message) {
  if (!source.includes(text)) failures.push(relative + ': ' + message);
}

const mainPath = 'firmware/esp32/src/main.cpp';
const stateHeaderPath = 'firmware/esp32/src/CM_JobStateStore.h';
const statePath = 'firmware/esp32/src/CM_JobStateStore.cpp';
const recoveryPath = 'firmware/esp32/src/CM_JobRecovery.cpp';
const guardPath = 'firmware/esp32/src/CM_BackupActivityGuard.cpp';
const persistenceAuditPath = 'firmware/esp32/src/CM_WindingSessionPersistenceIntegrityAudit.cpp';

const main = read(mainPath);
const stateHeader = read(stateHeaderPath);
const state = read(statePath);
const recovery = read(recoveryPath);
const guard = read(guardPath);
const persistenceAudit = read(persistenceAuditPath);

requireText(stateHeaderPath, stateHeader,
  'static bool isLocalPreparation(const JobRuntimeState& state);',
  'shared local-preparation predicate missing');
requireText(statePath, state,
  'state.deliveryState == JobDeliveryState::Created &&',
  'local preparation must require CREATED delivery');
requireText(statePath, state,
  'state.executionState == JobExecutionState::WaitingDelivery &&',
  'local preparation must require WAITING_DELIVERY execution');
requireText(statePath, state,
  'state.lastRunId == 0UL && state.completedRuns == 0U;',
  'local preparation must require zero physical-run evidence');
requireText(statePath, state,
  '!isLocalPreparation(latest)',
  'new-session admission must recognize only the shared local preparation predicate');
requireText(recoveryPath, recovery,
  'if (JobStateStore::isLocalPreparation(latest))',
  'recovery must recognize local preparation');
requireText(guardPath, guard,
  'const bool localPreparation = JobStateStore::isLocalPreparation(latest);',
  'backup activity guard must use the same local preparation predicate');

for (const text of [
  'snapshot.linkage.linked && !JobStateStore::isLocalPreparation(state)',
  '!hasSelections',
  '!selections.load(sessionId, selection, selectionFound)',
  '!selectionFound || !selection.isValid()',
  'selection.sessionId != sessionId',
  'selection.jobId != state.jobId',
  'selection.repairId != snapshot.linkage.repairId',
  'selection.motorId != snapshot.linkage.motorId'
]) {
  requireText(persistenceAuditPath, persistenceAudit, text,
    'linked post-preparation selection integrity guard missing: ' + text);
}

const snapshotPos = main.indexOf('jobSnapshots.create(job, linkage, createdMs)');
const stateCreatePos = main.indexOf('jobStates.create(job.jobId, job.sessionId, createdMs)');
const spoolCreatePos = main.indexOf('jobSpoolSelections.create(persistedSpoolSelection)');
const deliveringPos = main.indexOf('jobStates.updateDelivery(job.sessionId,\n                                  CM::JobDeliveryState::Delivering');
const queuePos = main.indexOf('receiver.queueJob(job)');

for (const [name, position] of [
  ['snapshot create', snapshotPos],
  ['CREATED state create', stateCreatePos],
  ['spool selection create', spoolCreatePos],
  ['DELIVERING transition', deliveringPos],
  ['UART queue', queuePos]
]) {
  if (position < 0) failures.push(mainPath + ': missing ' + name);
}

if (snapshotPos >= 0 && stateCreatePos >= 0 && snapshotPos >= stateCreatePos)
  failures.push(mainPath + ': snapshot must commit before CREATED runtime state');
if (stateCreatePos >= 0 && spoolCreatePos >= 0 && stateCreatePos >= spoolCreatePos)
  failures.push(mainPath + ': CREATED runtime state must commit before exact spool selection');
if (spoolCreatePos >= 0 && deliveringPos >= 0 && spoolCreatePos >= deliveringPos)
  failures.push(mainPath + ': exact spool selection must commit before DELIVERING');
if (deliveringPos >= 0 && queuePos >= 0 && deliveringPos >= queuePos)
  failures.push(mainPath + ': DELIVERING must commit before queueJob crosses UART boundary');

if (main.includes('jobStates.create(job.jobId, job.sessionId, createdMs) ||'))
  failures.push(mainPath + ': state create and DELIVERING transition must not be collapsed into one post-spool condition');

if (failures.length) {
  console.error(failures.join('\n'));
  process.exit(1);
}
console.log('JOB preparation transaction contracts OK: snapshot -> CREATED -> exact spool -> DELIVERING -> UART queue, safe local-preparation recovery only before the UART boundary, and mandatory linked spool-selection integrity afterwards.');
