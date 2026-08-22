const fs = require('fs');

const obsoleteImplPath = 'firmware/esp32/src/CM_WarehouseSpoolList.cpp';
const headerPath = 'firmware/esp32/src/CM_WarehouseStore.h';
const paginatedImplPath = 'firmware/esp32/src/CM_WarehouseSpoolMaterialList.cpp';
const webPath = 'firmware/esp32/src/CM_WarehouseSpoolWeb.cpp';

const failures = [];

if (fs.existsSync(obsoleteImplPath)) {
  failures.push(`${obsoleteImplPath}: obsolete non-paginated spool list backend must remain removed`);
}

const header = fs.readFileSync(headerPath, 'utf8');
const paginatedImpl = fs.readFileSync(paginatedImplPath, 'utf8');
const web = fs.readFileSync(webPath, 'utf8');

if (header.includes('appendActiveSpoolsJson(')) {
  failures.push(`${headerPath}: obsolete appendActiveSpoolsJson API declaration must remain removed`);
}

for (const required of [
  'appendActiveSpoolsPageJson(',
  'const char* materialFilter',
  'uint32_t cursor',
  'uint8_t limit',
]) {
  if (!header.includes(required)) failures.push(`${headerPath}: missing paginated spool API contract: ${required}`);
}

for (const required of [
  'bool WarehouseStore::appendActiveSpoolsPageJson',
  'WarehouseMaxListPageSize',
  'hasMore',
  'nextCursor',
]) {
  if (!paginatedImpl.includes(required)) failures.push(`${paginatedImplPath}: missing paginated spool backend contract: ${required}`);
}

for (const required of [
  'm_server.on("/api/warehouse/spools", HTTP_GET',
  'm_store.appendActiveSpoolsPageJson(',
  'invalid_paging_parameters',
  '\"has_more\"',
  '\"next_cursor\"',
]) {
  if (!web.includes(required)) failures.push(`${webPath}: spool endpoint must remain on paginated backend: ${required}`);
}

if (failures.length) {
  console.error(failures.join('\n'));
  process.exit(1);
}

console.log('Warehouse spool list cleanup contract OK: obsolete non-paginated backend/API stay removed and /api/warehouse/spools remains paginated.');
