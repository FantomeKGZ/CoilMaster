const fs = require('fs');
const path = require('path');

const repoRoot = path.resolve(__dirname, '..', '..');
const auditPath = path.join(repoRoot, 'firmware', 'esp32', 'src', 'CM_WarehousePersistenceIntegrityAudit.cpp');
const backupPath = path.join(repoRoot, 'firmware', 'esp32', 'src', 'CM_BackupExportWeb.cpp');

const audit = fs.readFileSync(auditPath, 'utf8');
const backup = fs.readFileSync(backupPath, 'utf8');

function requireText(source, text, description) {
  if (!source.includes(text)) {
    throw new Error(`Missing ${description}: ${text}`);
  }
}

function requirePattern(source, pattern, description) {
  if (!pattern.test(source)) {
    throw new Error(`Missing ${description}: ${pattern}`);
  }
}

requireText(
  audit,
  'bool WarehousePersistenceIntegrityAudit::check(fs::FS& storage)\n{\n    WarehousePersistenceAuditMetrics ignoredMetrics;\n    if (!check(storage, ignoredMetrics)) return false;\n\n    // Standalone callers retain the original broad integrity contract.\n    return checkMovementReferences(storage);\n}',
  'standalone broad warehouse persistence audit');

requireText(
  audit,
  'bool WarehousePersistenceIntegrityAudit::check(fs::FS& storage,\n                                               WarehousePersistenceAuditMetrics& metrics)',
  'metrics overload');

const metricsStart = audit.indexOf('bool WarehousePersistenceIntegrityAudit::check(fs::FS& storage,\n                                               WarehousePersistenceAuditMetrics& metrics)');
if (metricsStart < 0) throw new Error('Unable to locate metrics overload');
const metricsBody = audit.slice(metricsStart);

requireText(metricsBody, '!checkSpools(storage, spoolRecordCount)', 'scoped spool validation');
requireText(metricsBody, '!checkPrice(storage, priceRecordCount)', 'scoped price validation');
if (metricsBody.includes('checkMovementReferences(storage)')) {
  throw new Error('Metrics overload must not rescan movement references');
}

requireText(
  backup,
  'WarehousePersistenceIntegrityAudit::check(storage, warehouseMetrics)',
  'backup scoped warehouse persistence audit');
requirePattern(
  backup,
  /WarehouseMovementIntegrityAudit::check\(storage,\s*warehouseMovementRecordCount\)/,
  'separate authoritative warehouse movement audit');

console.log('Scoped warehouse backup audit contracts OK');
