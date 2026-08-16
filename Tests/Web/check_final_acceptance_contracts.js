const fs = require('fs');
const path = require('path');

const root = path.resolve(__dirname, '../..');
const failures = [];

function read(relative) {
  return fs.readFileSync(path.join(root, relative), 'utf8');
}

function requireText(relative, source, text, description) {
  if (!source.includes(text)) failures.push(relative + ': ' + description);
}

function requireFile(relative, description) {
  if (!fs.existsSync(path.join(root, relative))) failures.push(relative + ': ' + description);
}

const mainPath = 'firmware/esp32/src/main.cpp';
const registryPath = 'firmware/esp32/src/CM_RepairRegistryWeb.cpp';
const lookupPath = 'firmware/esp32/src/CM_RepairRegistryLookupWeb.cpp';
const warehousePath = 'firmware/esp32/src/CM_WarehouseWeb.cpp';
const writeOffPath = 'firmware/esp32/src/CM_WarehouseWriteOffWeb.cpp';
const backupPath = 'firmware/esp32/src/CM_RemoteBackupWeb.cpp';
const staticSitePath = 'firmware/esp32/src/CM_StaticSiteServer.cpp';
const storagePath = 'firmware/esp32/src/CM_StorageDiagnosticsWeb.cpp';
const releaseAuditPath = 'Tests/Web/check_release_contracts.js';
const webAuditPath = 'Tests/Web/check_web_assets.js';

const main = read(mainPath);
const registry = read(registryPath);
const lookup = read(lookupPath);
const warehouse = read(warehousePath);
const writeOff = read(writeOffPath);
const backup = read(backupPath);
const staticSite = read(staticSitePath);
const storage = read(storagePath);
const releaseAudit = read(releaseAuditPath);
const webAudit = read(webAuditPath);

// Populated-device data must remain available through bounded list APIs and exact lookups.
for (const route of ['/api/clients', '/api/motors', '/api/repairs']) {
  requireText(registryPath, registry, 'm_server.on("' + route + '", HTTP_GET',
    'required populated-device list route missing: ' + route);
}
requireText(registryPath, registry, 'limit = 20U;',
  'default bounded registry page size missing');
requireText(registryPath, registry, 'RepairRegistry::MaxListPageSize',
  'registry maximum page-size contract missing');
for (const route of ['/api/clients/by-id', '/api/motors/by-id', '/api/repairs/by-id']) {
  requireText(lookupPath, lookup, 'm_server.on("' + route + '", HTTP_GET',
    'required exact lookup route missing: ' + route);
}

// Warehouse visibility and exact linked-spool selection must remain part of production flow.
requireText(warehousePath, warehouse, 'm_server.on("/api/warehouse/summary", HTTP_GET',
  'warehouse summary route missing');
requireText(mainPath, main, 'linked_spool_id_required',
  'linked winding no longer requires an exact spool_id');
requireText(mainPath, main, 'loadActiveSpoolIdentity(spoolId, selectedSpool, spoolFound)',
  'linked winding no longer resolves exact ACTIVE spool identity');

// Runtime reboot/recovery contract stays fail-closed and observable.
requireText(mainPath, main, 'webServer.on("/api/status", HTTP_GET',
  'runtime status route missing');
requireText(mainPath, main, 'webServer.on("/api/system/diagnostics", HTTP_GET',
  'system diagnostics route missing');
requireText(mainPath, main, 'webServer.on("/api/system/time", HTTP_GET',
  'system time route missing');
requireText(mainPath, main,
  '\\"automatic_queue_allowed\\":false,\\"automatic_resume_allowed\\":false,\\"automatic_wire_writeoff_allowed\\":false',
  'no-auto-queue/resume/writeoff status contract missing');
requireText(staticSitePath, staticSite, 'm_server.on("/api/system/network", HTTP_GET',
  'network status route missing');

// microSD capacity remains operator-visible and strictly read-only.
requireText(storagePath, storage, 'm_server.on("/api/system/storage", HTTP_GET',
  'microSD diagnostics route missing');
for (const metric of ['cardSize()', 'totalBytes()', 'usedBytes()', 'automatic_cleanup_allowed\\\":false']) {
  requireText(storagePath, storage, metric, 'microSD diagnostics contract missing: ' + metric);
}
for (const forbidden of ['.remove(', '.rename(', 'FILE_WRITE', 'FILE_APPEND']) {
  if (storage.includes(forbidden)) failures.push(storagePath + ': diagnostics must remain read-only: ' + forbidden);
}

// Fresh backup must retain batch status + read-only inspection, while restore never auto-resumes.
for (const route of [
  '/api/backup/remote/batch-status',
  '/api/backup/remote/inspection',
  '/api/backup/remote/inspection-status'
]) {
  requireText(backupPath, backup, route, 'required backup acceptance route missing: ' + route);
}
requireText(backupPath, backup, 'm_server.arg("confirmed") != F("APPLY")',
  'restore explicit APPLY confirmation missing');
requireText(backupPath, backup, 'WAITING_RESTORE_CLEANUP',
  'post-reboot restore cleanup wait-state missing');
requireText(backupPath, backup, 'auto_resume=0',
  'restore result no longer explicitly prohibits auto-resume');

// Manual exact-run wire writeoff remains the only production deduction path.
for (const field of ['spool_id', 'source_session_id', 'source_run_id']) {
  requireText(writeOffPath, writeOff, '"' + field + '"',
    'manual writeoff provenance field missing: ' + field);
}
requireText(writeOffPath, writeOff, 'confirmedWriteOffForSourceRun(sourceSessionId,',
  'duplicate exact-run writeoff protection missing');

// Final operator UI surface must exist in both variants; generic web audit protects syntax/links.
for (const variant of ['desktop', 'mobile']) {
  for (const page of ['settings.html', 'repairs.html', 'motors.html', 'warehouse.html', 'winding-history.html', 'backup.html']) {
    requireFile('firmware/esp32/web/' + variant + '/' + page,
      'required final acceptance UI page missing');
  }
}
requireText(staticSitePath, staticSite, '/shared/settings-system-diagnostics.js',
  'settings diagnostics helper injection missing');
requireText(webAuditPath, webAudit, '/api/system/storage',
  'web audit no longer protects microSD diagnostics');

// The dedicated release audit remains the authoritative guard for physical START/SSR ownership.
for (const contract of [
  'physical START button polling contract missing',
  'direct SSR authority detected on ESP32',
  'duplicate source-run writeoff guard missing',
  'explicit APPLY confirmation contract missing'
]) {
  requireText(releaseAuditPath, releaseAudit, contract,
    'release safety audit lost required contract: ' + contract);
}

if (failures.length) {
  console.error(failures.join('\n'));
  process.exit(1);
}

console.log('Final acceptance contracts OK: bounded/exact workshop reads, warehouse and linked spool identity, diagnostics/storage/network/time, backup inspection, fail-closed restore, manual exact-run writeoff, and desktop/mobile acceptance UI.');
