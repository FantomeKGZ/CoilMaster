const fs = require('fs');
const path = require('path');

const root = path.resolve(__dirname, '../..');
const sourcePath = 'firmware/esp32/src/CM_AutonomousWindingWebCompleted.cpp';
const source = fs.readFileSync(path.join(root, sourcePath), 'utf8');
const failures = [];

function requireText(body, text, description) {
  if (!body.includes(text)) failures.push(`${sourcePath}: ${description}: ${text}`);
}

const pageStart = source.indexOf('bool AutonomousWindingArchive::appendCompletedWebJobsPageJson(');
const assignStart = source.indexOf('AutonomousWindingArchive::assignCompletedWebJobMotorChecked(', pageStart);
if (pageStart < 0 || assignStart < 0 || assignStart <= pageStart) {
  failures.push(`${sourcePath}: cannot isolate appendCompletedWebJobsPageJson()`);
} else {
  const page = source.slice(pageStart, assignStart);
  const stateReads = page.match(/JobStateStore::readPersisted\(/g) || [];
  if (stateReads.length !== 1) {
    failures.push(`${sourcePath}: completed ESP32 job page must read each candidate state only during the authoritative directory pass; found ${stateReads.length} JobStateStore::readPersisted() call sites`);
  }

  requireText(page,
    'WebJobPageItem selected[MaxTaskPageSize + 1U];',
    'completed-state snapshot must remain bounded to one page plus lookahead');
  requireText(page,
    'selected[insertAt].capture(state);',
    'validated candidate state must travel with bounded page selection');
  requireText(page,
    'const WebJobPageItem& state = selected[index];',
    'render loop must reuse the already validated bounded state snapshot');
  requireText(page,
    'JobSnapshotStore::readPersisted(m_storage, sessionId, snapshot)',
    'immutable job snapshot validation must remain authoritative during render');
  requireText(page,
    'state.completedRuns != snapshot.repeatTarget',
    'completed-runs versus immutable repeat target cross-check must remain');
  requireText(page,
    'state.deliveryState != JobDeliveryState::Accepted',
    'accepted delivery-state guard must remain');
}

requireText(source,
  'void capture(const JobRuntimeState& state)',
  'bounded page item must explicitly capture validated runtime state');
for (const field of ['jobId', 'lastRunId', 'updatedUptimeMs', 'completedRuns', 'deliveryState']) {
  requireText(source, `${field} = state.${field};`, `runtime state snapshot must retain ${field}`);
}

// Mutation boundary is intentionally different: assignment must still perform a
// fresh authoritative state read immediately before validating/appending linkage.
const mutation = source.indexOf('AutonomousWindingArchive::assignCompletedWebJobMotorChecked(');
if (mutation < 0) {
  failures.push(`${sourcePath}: assignCompletedWebJobMotorChecked() missing`);
} else {
  requireText(source.slice(mutation),
    'JobStateStore::readPersisted(m_storage, sessionId, state)',
    'assignment mutation must retain its TOCTOU-boundary state reread');
}

if (failures.length) {
  console.error(failures.join('\n'));
  process.exit(1);
}

console.log('Autonomous completed-job page contracts OK: runtime state is validated once per candidate and carried only in the bounded page snapshot; mutation-time reread remains intact.');
