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

must(registryHeader, 'bool updateMotor(uint32_t motorId, const NewMotor& motor);', 'registry update API');
must(registryHeader, 'motor-revisions.ndjson', 'append-only motor revision journal');
must(registryUpdate, 'FILE_APPEND', 'append-only motor revision write');
must(registryUpdate, '!motorExists(motorId, found) || !found', 'exact motor identity preservation');
must(registryUpdate, 'WindingProgramParser::canonicalize', 'winding program validation');
must(registryLookup, 'latestMotorRevisionLine(motorId', 'exact card latest revision read');
must(registryPage, 'latestMotorRevisionLine(motorId', 'catalog latest revision read');
must(motorWeb, '"/api/motors/update"', 'motor update endpoint');
must(motorWeb, 'm_registry.updateMotor(motorId, motor)', 'motor update persistence call');
mustNot(motorWeb, '/ssr', 'motor editor must not control SSR');

for (const surface of ['desktop', 'mobile']) {
  const editor = read(`firmware/esp32/web/${surface}/motor-edit.html`);
  const details = read(`firmware/esp32/web/${surface}/motor-details.html`);
  must(editor, '/api/motors/by-id?motor_id=', `${surface} editor exact load`);
  must(editor, '/api/motors/update', `${surface} editor save`);
  must(editor, "body.set('motor_id',motorId)", `${surface} editor fixed motor id`);
  must(details, `${surface}/motor-edit.html?motor_id=`.replace(`${surface}/`, `/${surface}/`), `${surface} detail edit link`);
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
console.log('Motor edit contracts OK: fixed motor_id revisions, exact reads, catalog overlay, desktop/mobile editors and Russian winding labels are present.');
