require('./check_dashboard_job_history.js');
require('./check_client_crm_ui.js');
const fs = require('fs');
const path = require('path');

const repoRoot = path.resolve(__dirname, '../..');
const webRoot = path.join(repoRoot, 'firmware/esp32/web');
const failures = [];

for (const relative of ['desktop/motor-details.html', 'mobile/motor-details.html']) {
  const source = fs.readFileSync(path.join(webRoot, relative), 'utf8');
  for (const token of ['/api/motors/by-id', '/api/motors/repairs', 'repeat_target', 'slot_count', 'manufacturer', 'model', 'не указано']) {
    if (!source.includes(token)) failures.push(`${relative}: missing ${token}`);
  }
  if (!source.includes('START')) failures.push(`${relative}: physical START repeat safety wording missing`);
  if (!source.includes('winding-history.html?repair_id=')) failures.push(`${relative}: winding history link missing`);
  if (!source.includes('costing.html?repair_id=')) failures.push(`${relative}: repair costing link missing`);
  if (!source.includes('has_more') || !source.includes('next_cursor')) failures.push(`${relative}: bounded repair history paging missing`);
}

const desktopDetails = fs.readFileSync(path.join(webRoot, 'desktop/motor-details.html'), 'utf8');
const roleSend = fs.readFileSync(path.join(webRoot, 'shared/motor-role-send.js'), 'utf8');
for (const token of [
  '/api/motors/winding/latest', '/api/motors/winding/versions',
  'Рабочая обмотка', 'Пусковая обмотка',
  'working_conductors', 'starting_conductors', 'legacy рабочая обмотка'
]) {
  if (!desktopDetails.includes(token)) failures.push(`desktop/motor-details.html: missing versioned winding contract ${token}`);
}
if (!desktopDetails.includes('versionNextCursor') || !desktopDetails.includes("limit:'12'")) {
  failures.push('desktop/motor-details.html: winding-version history is not bounded/paged');
}
if (!desktopDetails.includes('Web-действия никогда не включают SSR автоматически')) {
  failures.push('desktop/motor-details.html: explicit Web/SSR safety wording missing');
}
for (const token of ['/api/repairs/as-received', 'source_repair_id', 'При поступлении / после ремонта', 'snapshot_present', 'findVersionForRepair']) {
  if (!desktopDetails.includes(token)) failures.push(`desktop/motor-details.html: missing AS_RECEIVED comparison contract ${token}`);
}
if (!desktopDetails.includes('limit:"24"') || !desktopDetails.includes('invalid_paging_cursor')) {
  failures.push('desktop/motor-details.html: repair-version comparison must use bounded cursor paging');
}
if (!desktopDetails.includes('Legacy-ремонт: immutable AS_RECEIVED snapshot отсутствует')) {
  failures.push('desktop/motor-details.html: legacy repair without snapshot is not distinguished from corruption');
}
if (!desktopDetails.includes('Текущая карточка выше не подменяет исторический результат ремонта')) {
  failures.push('desktop/motor-details.html: current winding must not substitute missing historical repair result');
}
for (const token of [
  'winding-job.html?repair_id=', '&role=working', '&role=starting',
  'Отправить рабочую обмотку на станок', 'Отправить пусковую обмотку на станок'
]) {
  if (!desktopDetails.includes(token)) failures.push(`desktop/motor-details.html: missing safe OPEN-repair role job navigation ${token}`);
}
if (!desktopDetails.includes("closed?'':'<a href=\"/desktop/winding-job.html")) {
  failures.push('desktop/motor-details.html: linked role job links must be omitted for CLOSED repairs');
}
if (!desktopDetails.includes('/shared/motor-role-send.js')) {
  failures.push('desktop/motor-details.html: direct role-card send helper missing');
}
if (desktopDetails.includes("fetch('/api/jobs'")) {
  failures.push('desktop/motor-details.html: direct POST must stay isolated in the shared helper');
}

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
if (!roleSend.includes("body.item.starting_present !== true")) {
  failures.push('shared/motor-role-send.js: STARTING send must fail closed when authoritative STARTING is absent');
}
for (const forbidden of ['/api/motors/repairs?', 'repair_page_limit', 'windingJobUrl(', "form.set('repair_id'", "form.set('motor_id'", "form.set('spool_id'", '/api/ssr']) {
  if (roleSend.includes(forbidden)) failures.push(`shared/motor-role-send.js: forbidden direct-send dependency ${forbidden}`);
}

for (const relative of ['desktop/motors.html', 'mobile/motors.html']) {
  const source = fs.readFileSync(path.join(webRoot, relative), 'utf8');
  if (!source.includes('motor-details.html?motor_id=')) failures.push(`${relative}: motor details link missing`);
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

console.log('Motor details contracts OK: role cards can create unlinked service jobs directly from authoritative WORKING/STARTING data while repair-linked navigation remains separate and physical START/SSR/writeoff safety semantics are preserved.');
