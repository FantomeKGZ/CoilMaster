const fs = require('fs');

const headerPath = 'firmware/esp32/src/CM_WarehouseStore.h';
const implPath = 'firmware/esp32/src/CM_WarehouseLegacySpoolMaterial.cpp';
const webPath = 'firmware/esp32/src/CM_WarehouseSpoolWeb.cpp';

const header = fs.readFileSync(headerPath, 'utf8');
const impl = fs.readFileSync(implPath, 'utf8');
const web = fs.readFileSync(webPath, 'utf8');
const failures = [];

for (const required of [
  'bool assignLegacySpoolMaterial(uint32_t spoolId,const String& wireType);',
]) {
  if (!header.includes(required)) failures.push(`${headerPath}: missing active legacy spool material API: ${required}`);
}

for (const required of [
  'bool WarehouseStore::assignLegacySpoolMaterial',
  '(wireType != "CU" && wireType != "AL")',
  'hasMaterial',
  'status != "ACTIVE"',
  'return replaceSpoolsFileFromTemp();',
]) {
  if (!impl.includes(required)) failures.push(`${implPath}: missing safe legacy spool migration guard: ${required}`);
}

for (const required of [
  'm_server.on("/api/warehouse/spools/material", HTTP_POST',
  'handleAssignLegacySpoolMaterial();',
  'm_store.assignLegacySpoolMaterial(spoolId, material)',
  'wire_type_required_cu_or_al',
]) {
  if (!web.includes(required)) failures.push(`${webPath}: endpoint/store migration contract mismatch: ${required}`);
}

if (failures.length) {
  console.error(failures.join('\n'));
  process.exit(1);
}

console.log('Warehouse legacy spool material contract OK: active endpoint, CU/AL-only migration, ACTIVE unknown-material guard, and atomic spool replacement stay aligned.');
