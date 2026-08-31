const fs = require('fs');
const path = require('path');
const root = path.resolve(__dirname, '../..');
const read = rel => fs.readFileSync(path.join(root, rel), 'utf8');
const failures = [];
const must = (text, token, label) => { if (!text.includes(token)) failures.push(`${label}: missing ${token}`); };
const mustNot = (text, token, label) => { if (text.includes(token)) failures.push(`${label}: forbidden ${token}`); };

const registryHeader = read('firmware/esp32/src/CM_RepairRegistry.h');
const registryUpdate = read('firmware/esp32/src/CM_RepairRegistryMotorUpdate.cpp');
const registryLookup = read('firmware/esp32/src/CM_RepairRegistryLookup.cpp');
const registryPage = read('firmware/esp32/src/CM_RepairRegistryPage.cpp');
const motorWeb = read('firmware/esp32/src/CM_MotorSimilarityWeb.cpp');
const windingStore = read('firmware/esp32/src/CM_MotorWindingVersionStore.cpp');

must(registryHeader, 'bool updateMotor(uint32_t motorId, const NewMotor& motor);', 'registry update API');
must(registryHeader, 'motor-revisions.ndjson', 'append-only motor revision journal');
must(registryUpdate, 'FILE_APPEND', 'append-only motor revision write');
must(registryUpdate, '!motorExists(motorId, found) || !found', 'exact motor identity preservation');
must(registryUpdate, 'WindingProgramParser::canonicalize', 'winding program validation');
must(registryLookup, 'latestMotorRevisionLine(motorId', 'exact card latest revision read');
must(registryPage, 'latestMotorRevisionLine(motorId', 'catalog latest revision read');
must(motorWeb, '"/api/motors/update"', 'motor update endpoint');
must(motorWeb, 'm_registry.updateMotor(motorId, motor)', 'motor update persistence call');
must(motorWeb, '"/api/motors/winding/role"', 'winding role edit endpoint');
must(motorWeb, 'expected_winding_version_id', 'optimistic winding version token');
must(motorWeb, 'winding_version_conflict', 'stale winding version conflict');
must(motorWeb, 'm_server.send(409', 'winding conflict HTTP status');
must(motorWeb, 'next.previousVersionId = latestFound ? latestVersionId : 0UL;', 'append predecessor linkage');
must(motorWeb, 'next.versionKind = "MANUAL_ROLE_EDIT";', 'manual edit provenance');
must(motorWeb, 'if (roleName == "WORKING") next.working = replacement;', 'working-only replacement');
must(motorWeb, 'else next.starting = replacement;', 'starting-only replacement');
must(motorWeb, 'working_winding_version_required', 'starting cannot create orphan first version');
must(windingStore, 'FILE_APPEND', 'winding history remains append only');
mustNot(motorWeb, '/ssr', 'motor editor must not control SSR');

for (const surface of ['desktop', 'mobile']) {
  const editor = read(`firmware/esp32/web/${surface}/motor-edit.html`);
  const details = read(`firmware/esp32/web/${surface}/motor-details.html`);
  const catalog = read(`firmware/esp32/web/${surface}/motors.html`);
  must(editor, '/api/motors/by-id?motor_id=', `${surface} editor exact load`);
  must(editor, '/api/motors/update', `${surface} editor motor save`);
  must(editor, '/api/motors/winding/latest?motor_id=', `${surface} winding latest load`);
  must(editor, '/api/motors/winding/role', `${surface} winding role save`);
  must(editor, "b.set('expected_winding_version_id',String(currentWindingVersionId))", `${surface} expected winding version submit`);
  must(editor, "saveRole('WORKING')", `${surface} working edit action`);
  must(editor, "saveRole('STARTING')", `${surface} starting edit action`);
  must(editor, 'r.status===409', `${surface} conflict handling`);
  must(editor, 'await loadWinding()', `${surface} conflict/latest refresh`);
  must(editor, 'Рабочая обмотка', `${surface} Russian working label`);
  must(editor, 'Пусковая обмотка', `${surface} Russian starting label`);
  if (!editor.includes("body.set('motor_id',motorId)") &&
      !editor.includes("b.set('motor_id',motorId)")) {
    failures.push(`${surface} editor: fixed motor id missing`);
  }
  must(details, `${surface}/motor-edit.html?motor_id=`.replace(`${surface}/`, `/${surface}/`), `${surface} detail edit link`);
  must(catalog, `${surface}/motor-edit.html?motor_id=`.replace(`${surface}/`, `/${surface}/`), `${surface} catalog edit link`);
  must(catalog, '>Редактировать</a>', `${surface} catalog edit label`);
}

const desktopDetails = read('firmware/esp32/web/desktop/motor-details.html');
must(desktopDetails, 'Рабочая обмотка', 'desktop Russian working label');
must(desktopDetails, 'Пусковая обмотка', 'desktop Russian starting label');
mustNot(desktopDetails, '<h3>WORKING</h3>', 'desktop visible WORKING label');
mustNot(desktopDetails, '<h3>STARTING</h3>', 'desktop visible STARTING label');
mustNot(desktopDetails, 'Отправить WORKING на станок', 'desktop visible WORKING action');
mustNot(desktopDetails, 'Отправить STARTING на станок', 'desktop visible STARTING action');

if (failures.length) {
  console.error(failures.join('\n'));
  process.exit(1);
}
console.log('Motor edit contracts OK: fixed motor_id revisions, append-only WORKING/STARTING winding edits, conflict refresh, catalog/detail edit links and Russian role labels are present.');
