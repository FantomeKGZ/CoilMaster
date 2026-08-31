const fs = require('fs');
const path = require('path');

const root = path.resolve(__dirname, '../..');
const desktop = fs.readFileSync(path.join(root, 'firmware/esp32/web/desktop/arduino-windings.html'), 'utf8');
const mobile = fs.readFileSync(path.join(root, 'firmware/esp32/web/mobile/arduino-windings.html'), 'utf8');
const desktopHome = fs.readFileSync(path.join(root, 'firmware/esp32/web/desktop/index.html'), 'utf8');
const mobileHome = fs.readFileSync(path.join(root, 'firmware/esp32/web/mobile/index.html'), 'utf8');
const shared = fs.readFileSync(path.join(root, 'firmware/esp32/web/shared/arduino-windings-archive.js'), 'utf8');
const bridge = fs.readFileSync(path.join(root, 'firmware/esp32/web/shared/completed-web-job-archive-bridge.js'), 'utf8');
const compactIds = fs.readFileSync(path.join(root, 'firmware/esp32/web/shared/arduino-archive-compact-ids.js'), 'utf8');
const displayReset = fs.readFileSync(path.join(root, 'firmware/esp32/web/shared/completed-job-display-reset.js'), 'utf8');
const archiveHeader = fs.readFileSync(path.join(root, 'firmware/esp32/src/CM_AutonomousWindingArchive.h'), 'utf8');
const archiveWeb = fs.readFileSync(path.join(root, 'firmware/esp32/src/CM_AutonomousWindingWeb.cpp'), 'utf8');
const completedProjection = fs.readFileSync(path.join(root, 'firmware/esp32/src/CM_AutonomousWindingWebCompleted.cpp'), 'utf8');
const stateRead = fs.readFileSync(path.join(root, 'firmware/esp32/src/CM_JobStateReadOnly.cpp'), 'utf8');
const snapshotRead = fs.readFileSync(path.join(root, 'firmware/esp32/src/CM_JobSnapshotReadOnly.cpp'), 'utf8');

function requireText(text, needle, label) {
  if (!text.includes(needle)) throw new Error(`${label}: missing ${needle}`);
}

for (const [label, page] of [['desktop', desktop], ['mobile', mobile]]) {
  requireText(page, '/shared/completed-web-job-archive-bridge.js', `${label} completed-web bridge`);
  requireText(page, '/shared/arduino-windings-archive.js', label);
  requireText(page, '/shared/arduino-archive-compact-ids.js', `${label} compact ids`);
  if (page.indexOf('/shared/completed-web-job-archive-bridge.js') > page.indexOf('/shared/arduino-windings-archive.js')) throw new Error(`${label}: completed-web archive bridge must load before archive controller`);
  if (page.indexOf('/shared/arduino-archive-compact-ids.js') < page.indexOf('/shared/arduino-windings-archive.js')) throw new Error(`${label}: compact-id layer must load after archive controller`);
  for (const needle of ['id="bulkAssign"','id="createSelected"','id="combineSelected"','id="historyScan"','id="selectedCount"']) requireText(page, needle, label);
  requireText(page, '<option value="WORKING">', `${label} working role`);
  requireText(page, '<option value="STARTING">', `${label} starting role`);
  if (page.includes('value="AUXILIARY"')) throw new Error(`${label}: unsupported AUXILIARY role must not be present in static operator HTML`);
}
for (const [label, page] of [['desktop home', desktopHome], ['mobile home', mobileHome]]) {
  requireText(page, '/shared/completed-job-display-reset.js', label);
  requireText(page, 'current_job_cleared_after_completion', label);
}
requireText(desktop, 'class="table-wrap"', 'desktop compact table');
requireText(desktop, '.tip{', 'desktop accessible detail tooltip style');
requireText(shared, 'class="tip" tabindex="0" aria-label="Подробности"', 'desktop accessible detail tooltip markup');
requireText(shared, 'class="tip-box"', 'desktop accessible detail tooltip content');
requireText(mobile, 'details{', 'mobile accessible details style');
requireText(shared, '<details><summary>Подробности</summary>', 'mobile accessible details markup');

for (const needle of ['/api/autonomous-windings?','/api/autonomous-windings/assign','/api/motors?','/api/motors','session_id','run_id','repeat_target','completed_runs','historicalCounts',"status !== 'COMPLETED'"]) requireText(shared, needle, 'shared archive controller');
for (const needle of ['/api/autonomous-windings/web-completed?','/api/autonomous-windings/web-completed/assign',"source:item.source || 'ARDUINO_LOCAL'","source:'ESP32_JOB'",'archive_exact_run_source_collision']) requireText(bridge, needle, 'completed-web archive bridge');
requireText(shared, "confirmed:'true'", 'archive assignment owner must keep explicit confirmation');

for (const needle of ['setupAssignmentSafetyUi','replaceExistingControl','id="replaceExisting"','replacementRequested()','option.value !== \'WORKING\' && option.value !== \'STARTING\'',"body.set('replace_existing', replacementRequested() ? 'true' : 'false')"]) requireText(bridge, needle, 'explicit replacement safety UI');
requireText(bridge, "url.pathname === '/api/autonomous-windings/assign' ||", 'local assignment replacement gate');
requireText(bridge, "url.pathname === '/api/autonomous-windings/web-completed/assign'", 'completed web assignment replacement gate');
if (bridge.includes('motor_winding_role_occupied')) throw new Error('replacement UI must not auto-retry a role conflict; controller must surface the 409');
requireText(archiveWeb, 'motor_winding_role_occupied', 'backend occupied-role conflict');
requireText(archiveWeb, 'replace_existing_required', 'backend explicit replacement requirement');
requireText(archiveWeb, 'invalid_replace_existing', 'backend replacement flag validation');

for (const needle of ['dataset.exactSessionId','dataset.exactRunId','Session / Run:','№${ordinal}','MutationObserver','Exact Session / Run:']) requireText(compactIds, needle, 'compact operator id layer');
requireText(compactIds, 'observer.disconnect()', 'compact operator id observer feedback guard');
requireText(compactIds, 'applyWithoutObserverFeedback()', 'compact operator id guarded apply');
requireText(compactIds, "heading.textContent !== '№'", 'compact operator id idempotent heading');
if (compactIds.includes('fetch(') || compactIds.includes("method:'POST'") || compactIds.includes('/api/')) throw new Error('compact operator id layer must stay display-only');

for (const needle of ["body.job_status !== 'PROGRAM_COMPLETED'",'current_job_cleared_after_completion:true','last_completed_run_id:body.last_run_id',"job_status:'IDLE'",'job_id:0','run_active:false']) requireText(displayReset, needle, 'completed job display reset');
if (displayReset.includes("method:'POST'") || displayReset.includes('/api/jobs/cancel')) throw new Error('completed job display reset must stay read-only and must not mutate job state');

requireText(shared, 'Физические RUN events не изменялись', 'immutable linkage wording');
requireText(shared, 'read-only scan архива', 'bounded historical scan wording');
requireText(shared, "return Number.isInteger(value) && value > 0 ? value : '—'", 'repeat target must not be guessed');

for (const needle of ['appendCompletedWebJobsPageJson(','assignCompletedWebJobMotorChecked(','WebAssignmentsPath']) requireText(archiveHeader, needle, 'completed web job archive API');
for (const needle of ['m_server.on("/api/autonomous-windings/web-completed", HTTP_GET','m_server.on("/api/autonomous-windings/web-completed/assign", HTTP_POST','explicit_confirmation_required','assignCompletedWebJobMotorChecked(']) requireText(archiveWeb, needle, 'completed web job web API');
for (const needle of ['JobExecutionState::ProgramCompleted','JobDeliveryState::Accepted','state.completedRuns != snapshot.repeatTarget','JobStateStore::readPersisted','JobSnapshotStore::readPersisted','WebJobPageItem selected[MaxTaskPageSize + 1U]','snapshot.linkage.linked','source\\\":\\\"ESP32_JOB','exactAssignmentFound','exactMotorId == motorId && exactRole == role','return AutonomousWindingAssignResult::Invalid','if (assignmentFound[index])']) requireText(completedProjection, needle, 'completed web job projection/linkage');
if (completedProjection.includes('std::vector') || completedProjection.includes('readString()')) throw new Error('completed web job projection must not introduce unbounded collection or whole-directory buffering');
if (completedProjection.includes('EventsPath') || completedProjection.includes('WindingJournal')) throw new Error('completed web job projection must not copy or rewrite physical RUN evidence');

const pageStart = completedProjection.indexOf('bool AutonomousWindingArchive::appendCompletedWebJobsPageJson(');
const assignmentStart = completedProjection.indexOf('AutonomousWindingArchive::assignCompletedWebJobMotorChecked(', pageStart);
if (pageStart < 0 || assignmentStart <= pageStart) throw new Error('completed web job projection: cannot isolate read-only page function');
const completedPage = completedProjection.slice(pageStart, assignmentStart);
const pageStateReads = completedPage.match(/JobStateStore::readPersisted\(/g) || [];
if (pageStateReads.length !== 1) throw new Error(`completed web job projection: page must contain exactly one runtime-state read call site, found ${pageStateReads.length}`);
for (const needle of ['selected[insertAt].capture(state);','const WebJobPageItem& state = selected[index];','JobSnapshotStore::readPersisted(m_storage, sessionId, snapshot)','state.deliveryState != JobDeliveryState::Accepted','state.completedRuns != snapshot.repeatTarget']) requireText(completedPage, needle, 'completed web job bounded state reuse');
requireText(completedProjection, 'void capture(const JobRuntimeState& state)', 'completed web job bounded state capture');
for (const field of ['jobId','lastRunId','updatedUptimeMs','completedRuns','deliveryState']) requireText(completedProjection, `${field} = state.${field};`, `completed web job state capture ${field}`);
const assignmentMutation = completedProjection.slice(assignmentStart);
requireText(assignmentMutation, 'JobStateStore::readPersisted(m_storage, sessionId, state)', 'assignment mutation must keep TOCTOU-boundary state reread');

requireText(stateRead, '/data/winding-jobs/state/session-', 'read-only job state path');
requireText(snapshotRead, '/data/winding-jobs/snapshots/session-', 'read-only job snapshot path');

requireText(archiveHeader, 'AutonomousWindingAssignResult assignMotorChecked(uint32_t sessionId,', 'checked assignment API');
requireText(archiveWeb, 'assignMotorChecked(', 'Web must use checked assignment API');
const publicStart = archiveHeader.indexOf('public:');
const privateStart = archiveHeader.indexOf('private:');
if (publicStart < 0 || privateStart < 0 || privateStart <= publicStart) throw new Error('autonomous archive header: invalid public/private layout');
const publicApi = archiveHeader.slice(publicStart, privateStart);
const privateApi = archiveHeader.slice(privateStart);
for (const forbidden of ['completedTaskExists(', 'bool assignMotor(']) {
  if (publicApi.includes(forbidden)) throw new Error(`autonomous archive: legacy/internal API returned to public surface: ${forbidden}`);
  requireText(privateApi, forbidden, 'internal autonomous assignment helper');
}

console.log('Arduino archive UI contracts OK; compact Session/Run rendering is observer-feedback-safe, static and runtime role selectors are WORKING/STARTING only, occupied-role replacement is opt-in and never auto-retried, completed ESP32 jobs reuse one bounded validated runtime-state snapshot in read-only paging while mutation-time rereads remain intact, physical RUN evidence is not copied, and exact Session/Run provenance is preserved.');
