const fs = require('fs');

const source = fs.readFileSync('firmware/esp32/src/CM_MaterialPersistenceIntegrityAudit.cpp', 'utf8');
const header = fs.readFileSync('firmware/esp32/src/CM_MaterialPersistenceIntegrityAudit.h', 'utf8');
const backup = fs.readFileSync('firmware/esp32/src/CM_BackupExportWeb.cpp', 'utf8');

const failures = [];
function requireText(haystack, text, message) {
  if (!haystack.includes(text)) failures.push(message);
}

requireText(header,
  'static bool checkMaterialDomain(fs::FS& storage,',
  'scoped material-domain API missing');
requireText(source,
  'bool MaterialPersistenceIntegrityAudit::checkMaterialDomain(',
  'scoped material-domain implementation missing');
requireText(source,
  'if (!checkMaterialDomain(storage, ignoredMetrics)) return false;',
  'standalone audit no longer delegates through the scoped material domain');
requireText(source,
  'return WorkshopPersistenceIntegrityAudit::check(storage) &&\n           RepairPricingIntegrityAudit::check(storage);',
  'standalone material audit must retain broad workshop/pricing integrity checks');
requireText(source,
  'return checkMaterialDomain(storage, metrics);',
  'metrics overload must remain scoped for composite backup audit');
requireText(backup,
  'MaterialPersistenceIntegrityAudit::check(storage, materialMetrics)',
  'backup manifest no longer uses the scoped metrics overload');
requireText(backup,
  'BackupBusinessDataIntegrityAudit::check(storage, businessMetrics)',
  'backup manifest must still run authoritative business/pricing validation');

if (failures.length) {
  console.error(failures.join('\n'));
  process.exit(1);
}

console.log('Material backup scoped audit contracts OK: composite backup avoids transitive workshop/pricing duplication while standalone material integrity remains broad and fail-closed.');
