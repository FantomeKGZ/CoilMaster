const fs = require('fs');

const obsoleteImplPath = 'firmware/esp32/src/CM_WarehouseSpoolList.cpp';
const headerPath = 'firmware/esp32/src/CM_WarehouseStore.h';
const paginatedImplPath = 'firmware/esp32/src/CM_WarehouseSpoolMaterialList.cpp';
const webPath = 'firmware/esp32/src/CM_WarehouseSpoolWeb.cpp';
const warehouseWebPath = 'firmware/esp32/src/CM_WarehouseWeb.cpp';
const mainPath = 'firmware/esp32/src/main.cpp';

const failures = [];

if (fs.existsSync(obsoleteImplPath)) {
  failures.push(`${obsoleteImplPath}: obsolete non-paginated spool list backend must remain removed`);
}

const header = fs.readFileSync(headerPath, 'utf8');
const paginatedImpl = fs.readFileSync(paginatedImplPath, 'utf8');
const web = fs.readFileSync(webPath, 'utf8');
const warehouseWeb = fs.readFileSync(warehouseWebPath, 'utf8');
const main = fs.readFileSync(mainPath, 'utf8');

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
  'm_server.on("/api/warehouse/spools/material", HTTP_POST',
  'm_server.on("/api/warehouse/material-summary", HTTP_GET',
  'm_store.appendActiveSpoolsPageJson(',
  'invalid_paging_parameters',
  'has_more',
  'next_cursor',
]) {
  if (!web.includes(required)) failures.push(`${webPath}: spool endpoint must remain on paginated backend: ${required}`);
}

// beginSpoolList() is intentionally a narrow route owner. Common warehouse,
// write-off, conductor and material services are registered by begin() instead.
// Reintroducing them here would register production HTTP routes more than once
// because main.cpp calls both begin() and beginSpoolList().
for (const forbidden of [
  'beginWriteOff(',
  'ConductorCalculatorWeb',
  'ConductorSettingsWeb',
  'MaterialLedger',
  'MaterialLedgerWeb',
  'RepairRegistryWeb',
  'MotorSimilarityWeb',
]) {
  if (web.includes(forbidden)) {
    failures.push(`${webPath}: beginSpoolList owner must not duplicate common service bootstrap: ${forbidden}`);
  }
}

for (const required of [
  'beginWriteOff();',
  'ConductorCalculatorWeb conductorCalculatorWeb',
  'ConductorSettingsWeb conductorSettingsWeb',
  'MaterialLedger materialLedger',
]) {
  if (!warehouseWeb.includes(required)) {
    failures.push(`${warehouseWebPath}: common warehouse service bootstrap must remain owned by WarehouseWeb::begin(): ${required}`);
  }
}

for (const required of [
  'warehouseWeb.begin();',
  'warehouseWeb.beginSpoolList();',
]) {
  if (!main.includes(required)) {
    failures.push(`${mainPath}: production bootstrap must keep the split warehouse route owners explicit: ${required}`);
  }
}

if (failures.length) {
  console.error(failures.join('\n'));
  process.exit(1);
}

console.log('Warehouse spool list cleanup contract OK: paginated spool routes stay narrow and common warehouse services are registered only by WarehouseWeb::begin().');
