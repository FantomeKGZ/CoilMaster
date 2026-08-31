const fs = require('fs');

const source = fs.readFileSync('firmware/esp32/src/CM_MaterialPersistenceIntegrityAudit.cpp', 'utf8');
const header = fs.readFileSync('firmware/esp32/src/CM_MaterialPersistenceIntegrityAudit.h', 'utf8');
const backup = fs.readFileSync('firmware/esp32/src/CM_BackupExportWeb.cpp', 'utf8');

const failures = [];
function requireText(haystack, text, message) {
  if (!haystack.includes(text)) failures.push(message);
}
function forbidText(haystack, text, message) {
  if (haystack.includes(text)) failures.push(message);
}

requireText(header,
  'static bool checkMaterialDomain(fs::FS& storage,',
  'scoped material-domain API missing');
requireText(source,
  'bool MaterialPersistenceIntegrityAudit::checkMaterialDomain(',
  'scoped material-domain implementation missing');
requireText(source,
  '!checkMaterialDomain(storage, ignoredMetrics)',
  'standalone audit no longer delegates through the scoped material domain');
requireText(source,
  'workshopDirectoryShapeValid(storage)',
  'standalone audit must retain fail-closed /data/workshop directory-shape validation');
requireText(source,
  'BackupBusinessDataIntegrityAudit::check(storage)',
  'standalone audit must use authoritative workshop/pricing validation');
requireText(source,
  'WarehousePersistenceIntegrityAudit::check(storage)',
  'standalone audit must retain broad warehouse validation');
requireText(source,
  'PersistentIdIntegrityAudit::check(storage)',
  'standalone audit must retain allocator integrity validation');
requireText(source,
  'WindingSessionPersistenceIntegrityAudit::check(storage)',
  'standalone audit must retain winding-session persistence validation');
requireText(source,
  'RunWireAccountingIntegrityAudit::check(storage)',
  'standalone audit must retain exact RUN_WIRE cross-log accounting validation');
requireText(source,
  'WindingPersistenceIntegrityAudit::check(storage)',
  'standalone audit must retain winding journal validation');
forbidText(source,
  'WorkshopPersistenceIntegrityAudit',
  'standalone material audit must not depend on the retired workshop wrapper');
forbidText(source,
  'RepairPricingIntegrityAudit',
  'standalone material audit must not depend on the duplicate pricing audit owner');
requireText(source,
  'return checkMaterialDomain(storage, metrics);',
  'metrics overload must remain scoped for composite backup audit');
requireText(backup,
  'MaterialPersistenceIntegrityAudit::check(storage, materialMetrics)',
  'backup manifest no longer uses the scoped metrics overload');
requireText(backup,
  'BackupBusinessDataIntegrityAudit::check(storage, businessMetrics)',
  'backup manifest must still run authoritative business/pricing validation');
requireText(backup,
  '#include "CM_RunWireAccountingIntegrityAudit.h"',
  'backup snapshot gate must include the authoritative RUN_WIRE accounting audit');
requireText(backup,
  'RunWireAccountingIntegrityAudit::check(storage)',
  'backup snapshot gate must directly validate RUN_WIRE cross-log accounting');
requireText(backup,
  'run_wire_accounting_unstable_or_invalid',
  'RUN_WIRE accounting failure must fail backup stability closed with a distinct reason');

if (failures.length) {
  console.error(failures.join('\n'));
  process.exit(1);
}

console.log('Material backup scoped audit contracts OK: composite backup stays scoped and directly validates RUN_WIRE, while standalone material integrity uses authoritative direct domain owners without legacy wrapper/pricing duplicates.');
