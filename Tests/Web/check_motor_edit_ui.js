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
const windingHeader = read('firmware/esp32/src/CM_MotorWindingVersionStore.h');
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
must(motorWeb, 'conductor.materialClass != "CU" && conductor.materialClass != "AL"', 'canonical copper/aluminum validation');
must(windingHeader, 'static constexpr uint8_t MaxConductors = 5U;', 'five canonical conductor components');
must(windingStore, 'role.conductorCount > MotorWindingRoleSpec::MaxConductors', 'canonical conductor limit uses shared model constant');
must(windingStore, 'FILE_APPEND', 'winding history remains append only');
mustNot(motorWeb, '/ssr', 'motor editor must not control SSR');

for (const surface of ['desktop', 'mobile']) {
  const creator = read(`firmware/esp32/web/${surface}/motor-new.html`);
  const editor = read(`firmware/esp32/web/${surface}/motor-edit.html`);
  const details = read(`firmware/esp32/web/${surface}/motor-details.html`);
  const catalog = read(`firmware/esp32/web/${surface}/motors.html`);

  must(creator, 'Рабочая обмотка', `${surface} new motor working role`);
  must(creator, 'Пусковая обмотка', `${surface} new motor starting role`);
  must(creator, 'has_starting', `${surface} optional starting toggle`);
  must(creator, 'working_material', `${surface} working material selector`);
  must(creator, 'starting_material', `${surface} starting material selector`);
  must(creator, '>Медь</option>', `${surface} copper option`);
  must(creator, '>Алюминий</option>', `${surface} aluminum option`);
  must(creator, 'working_wires', `${surface} working wire list`);
  must(creator, 'starting_wires', `${surface} starting wire list`);
  must(creator, 'split(/[;:]/)', `${surface} semicolon/colon wire separators`);
  must(creator, '>5', `${surface} five-wire input bound`);
  must(creator, 'replace(\',\',\'.\')', `${surface} comma decimal normalization`);
  must(creator, 'x${', `${surface} duplicate strand aggregation canonical form`);
  must(creator, '/api/motors/winding/role', `${surface} new motor canonical winding endpoint`);
  must(creator, "'WORKING',0", `${surface} WORKING first version`);
  must(creator, "'STARTING',version", `${surface} STARTING follows WORKING version`);
  must(creator, "$('coil_program').value=working.program", `${surface} legacy program derives from WORKING`);
  mustNot(creator, '/ssr', `${surface} new motor must not control SSR`);

  must(editor, '/api/motors/by-id?motor_id=', `${surface} editor exact load`);
  must(editor, '/api/motors/update', `${surface} editor motor save`);
  must(editor, '/api/motors/winding/latest?motor_id=', `${surface} winding latest load`);
  must(editor, '/api/motors/winding/role', `${surface} winding role save`);
  must(editor, "b.set('expected_winding_version_id',String(currentWindingVersionId))", `${surface} expected winding version submit`);
  must(editor, "saveRole('WORKING')", `${surface} working edit action`);
  must(editor, "saveRole('STARTING')", `${surface} starting edit action`);
  must(editor, 'r.status===409', `${surface} conflict handling`);
  must(editor, 'await loadWinding()', `${surface} conflict/latest refresh`);
  must(editor, 'working_material', `${surface} working material edit`);
  must(editor, 'starting_material', `${surface} starting material edit`);
  must(editor, 'working_wires', `${surface} working friendly wire edit`);
  must(editor, 'starting_wires', `${surface} starting friendly wire edit`);
  must(editor, 'working_conductors_raw', `${surface} working raw canonical fallback`);
  must(editor, 'starting_conductors_raw', `${surface} starting raw canonical fallback`);
  must(editor, 'parseCanonicalConductors', `${surface} canonical-to-friendly conversion`);
  must(editor, 'canonicalConductors', `${surface} friendly-to-canonical conversion`);
  must(editor, 'split(/[;:]/)', `${surface} editor semicolon/colon wire separators`);
  must(editor, 'wires.length+count>5', `${surface} canonical physical wire bound`);
  must(editor, 'Рабочая обмотка', `${surface} Russian working label`);
  must(editor, 'Пусковая обмотка', `${surface} Russian starting label`);
  mustNot(editor, '/ssr', `${surface} editor must not control SSR`);
  if (!editor.includes("body.set('motor_id',motorId)") &&
      !editor.includes("b.set('motor_id',motorId)")) {
    failures.push(`${surface} editor: fixed motor id missing`);
  }

  must(details, `${surface}/motor-edit.html?motor_id=`.replace(`${surface}/`, `/${surface}/`), `${surface} detail edit link`);
  must(details, 'Материал / провод', `${surface} human winding material/wire label`);
  must(details, 'formatConductors', `${surface} canonical conductor display formatter`);
  must(details, "m[1]==='CU'?'Медь':'Алюминий'", `${surface} human copper/aluminum display`);
  must(catalog, `${surface}/motor-edit.html?motor_id=`.replace(`${surface}/`, `/${surface}/`), `${surface} catalog edit link`);
  must(catalog, '>Редактировать</a>', `${surface} catalog edit label`);
}

const desktopCatalog = read('firmware/esp32/web/desktop/motors.html');
must(desktopCatalog, "values.push('Рабочая: '+esc(version.working_conductors))", 'desktop catalog working conductor escaping');
must(desktopCatalog, "values.push('Пусковая: '+esc(version.starting_conductors))", 'desktop catalog starting conductor escaping');
const mobileCatalog = read('firmware/esp32/web/mobile/motors.html');
must(mobileCatalog, "esc(conductors)", 'mobile catalog conductor escaping');

const desktopDetails = read('firmware/esp32/web/desktop/motor-details.html');
mustNot(desktopDetails, '<h3>WORKING</h3>', 'desktop visible WORKING label');
mustNot(desktopDetails, '<h3>STARTING</h3>', 'desktop visible STARTING label');
mustNot(desktopDetails, 'Отправить WORKING на станок', 'desktop visible WORKING action');
mustNot(desktopDetails, 'Отправить STARTING на станок', 'desktop visible STARTING action');

if (failures.length) {
  console.error(failures.join('\n'));
  process.exit(1);
}
console.log('Motor winding UI contracts OK: append-only WORKING/STARTING edits, new-motor copper/aluminum and 1-5 wire capture, and catalog conductor escaping are present on desktop/mobile.');
