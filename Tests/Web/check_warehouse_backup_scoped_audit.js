const fs = require('fs');
const path = require('path');

const repoRoot = path.resolve(__dirname, '..', '..');
const auditPath = path.join(repoRoot, 'firmware', 'esp32', 'src', 'CM_WarehousePersistenceIntegrityAudit.cpp');
const bridgeAuditPath = path.join(repoRoot, 'firmware', 'esp32', 'src', 'CM_SpoolMaterialBridgeIntegrityAudit.cpp');
const bridgeHeaderPath = path.join(repoRoot, 'firmware', 'esp32', 'src', 'CM_SpoolMaterialBridgeIntegrityAudit.h');
const backupPath = path.join(repoRoot, 'firmware', 'esp32', 'src', 'CM_BackupExportWeb.cpp');

const audit = fs.readFileSync(auditPath, 'utf8');
const bridgeAudit = fs.readFileSync(bridgeAuditPath, 'utf8');
const bridgeHeader = fs.readFileSync(bridgeHeaderPath, 'utf8');
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
requireText(metricsBody, '!SpoolMaterialBridgeIntegrityAudit::check(storage, bridgeRecordCount)', 'scoped spool bridge validation');
requireText(metricsBody, 'metrics.spoolMaterialBridgeRecordCount = bridgeRecordCount;', 'spool bridge metric');
if (metricsBody.includes('checkMovementReferences(storage)')) {
  throw new Error('Metrics overload must not rescan movement references');
}

requireText(bridgeHeader, 'static bool check(fs::FS& storage, uint32_t& recordCount);', 'read-only bridge audit API');
for (const token of [
  'constexpr uint8_t ReferenceBatchSize = 24U;',
  'SpoolMaterialBridgeStore store(storage);',
  'if (!store.validateAll()) return false;',
  'diameter != reference.diameterHundredthsMm',
  'wireType != reference.wireType',
  'unit != "GRAM"',
  'resolveSpoolReferences(storage, references, count)',
  'resolveMaterialReferences(storage, references, count)'
]) requireText(bridgeAudit, token, 'bridge cross-reference integrity');

requireText(
  backup,
  'WarehousePersistenceIntegrityAudit::check(storage, warehouseMetrics)',
  'backup scoped warehouse persistence audit');
requirePattern(
  backup,
  /WarehouseMovementIntegrityAudit::check\(storage,\s*warehouseMovementRecordCount\)/,
  'separate authoritative warehouse movement audit');
requireText(
  backup,
  '{"spool-material-bridges", "/data/warehouse/spool-material-bridges.ndjson", "application/x-ndjson", "spool-material-bridges.ndjson"}',
  'spool bridge backup export');
requireText(backup, 'spoolMaterialBridgeRecordCount = 0UL;', 'backup bridge count metric storage');
requireText(backup, 'metrics.spoolMaterialBridgeRecordCount = warehouseMetrics.spoolMaterialBridgeRecordCount;', 'backup bridge metric assignment');
requireText(backup, 'spool_material_bridge_record_count', 'backup bridge metric response');

console.log('Scoped warehouse backup audit contracts OK: spool bridge export and bounded exact cross-reference integrity are included without duplicating movement scans.');
