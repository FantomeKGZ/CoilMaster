require('./check_dashboard_job_history.js');
require('./check_client_crm_ui.js');
const fs = require('fs');
const path = require('path');

const repoRoot = path.resolve(__dirname, '../..');
const webRoot = path.join(repoRoot, 'firmware/esp32/web');
const failures = [];

const detailsPages = ['desktop/motor-details.html', 'mobile/motor-details.html'];
for (const relative of detailsPages) {
  const source = fs.readFileSync(path.join(webRoot, relative), 'utf8');
  for (const token of [
    '/api/motors/by-id', '/api/motors/repairs', 'repeat_target', 'slot_count',
    'manufacturer', 'model', 'не указано', '/api/motors/winding/latest',
    '/api/motors/winding/versions', 'Рабочая обмотка', 'Пусковая обмотка',
    'working_conductors', 'starting_conductors', '/api/repairs/as-received',
    'source_repair_id', 'При поступлении / после ремонта', 'snapshot_present',
    'findVersionForRepair', 'Legacy-ремонт: immutable AS_RECEIVED snapshot отсутствует',
    'Текущая карточка выше не подменяет исторический результат ремонта',
    '/shared/motor-role-send.js'
  ]) {
    if (!source.includes(token)) failures.push(`${relative}: missing parity contract ${token}`);
  }
  if (!source.includes('START')) failures.push(`${relative}: physical START repeat safety wording missing`);
  if (!source.includes('никогда не включают SSR автоматически')) failures.push(`${relative}: Web/SSR safety wording missing`);
  if (!source.includes('winding-history.html?repair_id=')) failures.push(`${relative}: winding history link missing`);
  if (!source.includes('costing.html?repair_id=')) failures.push(`${relative}: repair costing link missing`);
  if (!source.includes('has_more') || !source.includes('next_cursor')) failures.push(`${relative}: bounded repair history paging missing`);
  if (!source.includes('versionNextCursor') || !source.includes("limit:'12'")) failures.push(`${relative}: winding-version history is not bounded/paged`);
  if (!source.includes("limit:'24'") || !source.includes('invalid_paging_cursor')) failures.push(`${relative}: repair-version comparison must use bounded cursor paging`);
  if (source.includes("fetch('/api/jobs'")) failures.push(`${relative}: direct POST must stay isolated in shared helper`);
}

const desktopDetails = fs.readFileSync(path.join(webRoot, 'desktop/motor-details.html'), 'utf8');
const mobileDetails = fs.readFileSync(path.join(webRoot, 'mobile/motor-details.html'), 'utf8');
for (const [relative, source, prefix] of [
  ['desktop/motor-details.html', desktopDetails, '/desktop/'],
  ['mobile/motor-details.html', mobileDetails, '/mobile/']
]) {
  for (const token of [
    `winding-job.html?repair_id=`, '&role=working', '&role=starting'
  ]) {
    if (!source.includes(token)) failures.push(`${relative}: missing repair-linked role navigation ${token}`);
  }
  if (!source.includes(prefix + 'motor-edit.html?motor_id=')) failures.push(`${relative}: motor edit navigation missing`);
}

const roleSend = fs.readFileSync(path.join(webRoot, 'shared/motor-role-send.js'), 'utf8');
for (const token of [
  'Отправить на станок', '/api/motors/winding/latest?motor_id=', '/api/motors/by-id?motor_id=',
  '/api/status', 'job_creation_ready === true', "form.set('type', role)",
  "form.set('turns', selected.program)", "form.set('repeat_target', String(selected.repeat))",
  "jsonFetch('/api/jobs', {method:'POST', body:form})", 'body.linked === false',
  'body.repair_id === null', 'body.motor_id === null', 'body.spool_id === null',
  'body.spool_selection_saved === false', 'body.automatic_wire_writeoff_allowed === false',
  'Физический START остаётся обязательным', 'Запуск — только физической кнопкой START'
]) {
  if (!roleSend.includes(token)) failures.push(`shared/motor-role-send.js: missing direct service-send contract ${token}`);
}
if (!roleSend.includes("body.item.starting_present !== true")) failures.push('shared/motor-role-send.js: STARTING send must fail closed when authoritative STARTING is absent');
for (const forbidden of ['/api/motors/repairs?', 'repair_page_limit', 'windingJobUrl(', "form.set('repair_id'", "form.set('motor_id'", "form.set('spool_id'", '/api/ssr']) {
  if (roleSend.includes(forbidden)) failures.push(`shared/motor-role-send.js: forbidden direct-send dependency ${forbidden}`);
}

for (const relative of ['desktop/motors.html', 'mobile/motors.html']) {
  const source = fs.readFileSync(path.join(webRoot, relative), 'utf8');
  if (!source.includes('motor-details.html?motor_id=')) failures.push(`${relative}: motor details link missing`);
  for (const token of ['/api/motors/winding/latest?motor_id=', 'working_program', 'starting_present', 'starting_program']) {
    if (!source.includes(token)) failures.push(`${relative}: versioned WORKING/STARTING catalogue parity missing ${token}`);
  }
}

const mobileEdit = fs.readFileSync(path.join(webRoot, 'mobile/motor-edit.html'), 'utf8');
for (const token of ['/api/motors/winding/latest?motor_id=', '/api/motors/winding/role', 'saveWorking', 'saveStarting', 'expected_winding_version_id']) {
  if (!mobileEdit.includes(token)) failures.push(`mobile/motor-edit.html: edit parity missing ${token}`);
}

const registryHeader = fs.readFileSync(path.join(repoRoot, 'firmware/esp32/src/CM_RepairRegistry.h'), 'utf8');
const registryPage = fs.readFileSync(path.join(repoRoot, 'firmware/esp32/src/CM_RepairRegistryPage.cpp'), 'utf8');
const lookupHeader = fs.readFileSync(path.join(repoRoot, 'firmware/esp32/src/CM_RepairRegistryLookupWeb.h'), 'utf8');
const lookupSource = fs.readFileSync(path.join(repoRoot, 'firmware/esp32/src/CM_RepairRegistryLookupWeb.cpp'), 'utf8');
if (!registryHeader.includes('uint32_t motorId')) failures.push('CM_RepairRegistry.h: motorId repair-page filter missing');
if (!registryHeader.includes('0UL,\n                                     statusFilter')) failures.push('CM_RepairRegistry.h: backward-compatible repair paging overload missing');
if (!registryPage.includes('lineMotorId != motorId')) failures.push('CM_RepairRegistryPage.cpp: exact motor_id repair filter missing');
if (!lookupHeader.includes('handleMotorRepairs')) failures.push('CM_RepairRegistryLookupWeb.h: motor repair handler missing');
if (!lookupSource.includes('/api/motors/repairs')) failures.push('CM_RepairRegistryLookupWeb.cpp: motor repair endpoint missing');
if (!lookupSource.includes('/api/repairs/as-received')) failures.push('CM_RepairRegistryLookupWeb.cpp: AS_RECEIVED endpoint missing');
if (!lookupSource.includes('appendRepairsPageJson(response,\n                                          0UL,\n                                          motorId')) failures.push('CM_RepairRegistryLookupWeb.cpp: endpoint does not use exact motor_id paging');
if (!lookupSource.includes('MaxListPageSize')) failures.push('CM_RepairRegistryLookupWeb.cpp: motor repair endpoint is not bounded');

if (failures.length) {
  console.error(failures.join('\n'));
  process.exit(1);
}
console.log('Motor desktop/mobile parity OK: versioned WORKING/STARTING cards, direct service-send helper, bounded winding/repair history, AS_RECEIVED comparison, edit workflow and catalogue role views are present while physical START/SSR/writeoff safety semantics remain preserved.');
