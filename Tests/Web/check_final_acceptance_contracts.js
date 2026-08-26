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
const writeOffStorePath = 'firmware/esp32/src/CM_WarehouseWriteOff.cpp';
const writeOffRecoveryPath = 'firmware/esp32/src/CM_WarehouseWriteOffRecovery.cpp';
const warehouseHeaderPath = 'firmware/esp32/src/CM_WarehouseStore.h';
const runWirePath = 'firmware/esp32/src/CM_RunWireIssueCoordinator.cpp';
const backupPath = 'firmware/esp32/src/CM_RemoteBackupWeb.cpp';
const backupGuardPath = 'firmware/esp32/src/CM_BackupActivityGuard.cpp';
const backupExportPath = 'firmware/esp32/src/CM_BackupExportWeb.cpp';
const sessionAuditPath = 'firmware/esp32/src/CM_WindingSessionPersistenceIntegrityAudit.cpp';
const sessionAuditHeaderPath = 'firmware/esp32/src/CM_WindingSessionPersistenceIntegrityAudit.h';
const staticSitePath = 'firmware/esp32/src/CM_StaticSiteServer.cpp';
const networkWebPath = 'firmware/esp32/src/CM_NetworkWeb.cpp';
const storagePath = 'firmware/esp32/src/CM_StorageDiagnosticsWeb.cpp';
const releaseAuditPath = 'Tests/Web/check_release_contracts.js';
const webAuditPath = 'Tests/Web/check_web_assets.js';

const main = read(mainPath);
const registry = read(registryPath);
const lookup = read(lookupPath);
const warehouse = read(warehousePath);
const writeOff = read(writeOffPath);
const writeOffStore = read(writeOffStorePath);
const writeOffRecovery = read(writeOffRecoveryPath);
const warehouseHeader = read(warehouseHeaderPath);
const runWire = read(runWirePath);
const backup = read(backupPath);
const backupGuard = read(backupGuardPath);
const backupExport = read(backupExportPath);
const sessionAudit = read(sessionAuditPath);
const sessionAuditHeader = read(sessionAuditHeaderPath);
const staticSite = read(staticSitePath);
const networkWeb = read(networkWebPath);
const storage = read(storagePath);
const releaseAudit = read(releaseAuditPath);
const webAudit = read(webAuditPath);

for (const route of ['/api/clients', '/api/motors', '/api/repairs']) {
  requireText(registryPath, registry, 'm_server.on("' + route + '", HTTP_GET', 'required populated-device list route missing: ' + route);
}
requireText(registryPath, registry, 'limit = 20U;', 'default bounded registry page size missing');
requireText(registryPath, registry, 'RepairRegistry::MaxListPageSize', 'registry maximum page-size contract missing');
for (const route of ['/api/clients/by-id', '/api/motors/by-id', '/api/repairs/by-id']) {
  requireText(lookupPath, lookup, 'm_server.on("' + route + '", HTTP_GET', 'required exact lookup route missing: ' + route);
}

requireText(warehousePath, warehouse, 'm_server.on("/api/warehouse/summary", HTTP_GET', 'warehouse summary route missing');
requireText(mainPath, main, 'linked_spool_id_required', 'linked winding no longer requires an exact spool_id');
requireText(mainPath, main, 'loadActiveSpoolIdentity(spoolId, selectedSpool, spoolFound)', 'linked winding no longer resolves exact ACTIVE spool identity');

requireText(mainPath, main, 'webServer.on("/api/status", HTTP_GET', 'runtime status route missing');
requireText(mainPath, main, 'webServer.on("/api/system/diagnostics", HTTP_GET', 'system diagnostics route missing');
requireText(mainPath, main, 'webServer.on("/api/system/time", HTTP_GET', 'system time route missing');
requireText(mainPath, main,
  '\\"automatic_queue_allowed\\":false,\\"automatic_resume_allowed\\":false,\\"automatic_wire_writeoff_allowed\\":false',
  'no-auto-queue/resume/writeoff status contract missing');
requireText(staticSitePath, staticSite, 'm_server.on("/api/system/network", HTTP_GET', 'network status route missing');

for (const text of [
  '"{\\"error\\":\\"network_profiles_unavailable\\"}"',
  '"{\\"error\\":\\"network_profile_not_found\\"}"',
  '"{\\"error\\":\\"network_profile_capacity_reached\\"}"',
  '"{\\"error\\":\\"network_profile_persistence_failed\\"}"',
  '"{\\"error\\":\\"network_profile_delete_persistence_failed\\"}"',
  '"{\\"error\\":\\"network_manager_reload_failed\\"}"'
]) requireText(networkWebPath, networkWeb, text, 'network API error-semantics contract missing: ' + text);
requireText(networkWebPath, networkWeb, 'm_server.send(m_store.ready() ? 500 : 503,',
  'network persistence failures must distinguish store unavailable from write failure');

requireText(storagePath, storage, 'm_server.on("/api/system/storage", HTTP_GET', 'microSD diagnostics route missing');
for (const metric of ['cardSize()', 'totalBytes()', 'usedBytes()', 'automatic_cleanup_allowed\\\":false']) {
  requireText(storagePath, storage, metric, 'microSD diagnostics contract missing: ' + metric);
}
for (const forbidden of ['.remove(', '.rename(', 'FILE_WRITE', 'FILE_APPEND']) {
  if (storage.includes(forbidden)) failures.push(storagePath + ': diagnostics must remain read-only: ' + forbidden);
}

for (const route of ['/api/backup/remote/batch-status', '/api/backup/remote/inspection', '/api/backup/remote/inspection-status']) {
  requireText(backupPath, backup, route, 'required backup acceptance route missing: ' + route);
}
requireText(backupPath, backup, 'm_server.arg("confirmed") != F("APPLY")', 'restore explicit APPLY confirmation missing');
requireText(backupPath, backup, 'WAITING_RESTORE_CLEANUP', 'post-reboot restore cleanup wait-state missing');
requireText(backupPath, backup, 'auto_resume=0', 'restore result no longer explicitly prohibits auto-resume');

for (const text of [
  'const BackupActivityCheck runtime = runtimeCheck();',
  'if (runtime != BackupActivityCheck::Safe)\n        return runtime;',
  'if (!found)\n        return BackupActivityCheck::Safe;'
]) requireText(backupGuardPath, backupGuard, text, 'backup activity runtime fail-closed contract missing: ' + text);
if (backupGuard.includes('runtime == BackupActivityCheck::Unavailable\n            ? BackupActivityCheck::Safe')) {
  failures.push(backupGuardPath + ': unavailable runtime must never be promoted to Safe');
}

for (const text of [
  'enum class WindingSessionPersistenceAuditFailure', 'TemporaryFilePresent', 'InvalidDirectoryEntry',
  'directoryPreflightMeasured', 'directoryPreflightDurationMs'
]) requireText(sessionAuditHeaderPath, sessionAuditHeader, text, 'session persistence preflight result contract missing: ' + text);
for (const text of [
  'directoryContentsCanonical(storage, SelectionDirectory)', 'isCanonicalTempName(name)',
  'const uint32_t preflightStartedAtMs = millis();', 'validatedMetrics.directoryPreflightMeasured = true;',
  'JobSnapshotStore snapshots(storage);', 'JobSpoolSelectionStore selections(storage);'
]) requireText(sessionAuditPath, sessionAudit, text, 'selection-only read-only session persistence preflight missing: ' + text);
for (const forbidden of [
  'const char* directories[] = {SnapshotDirectory, StateDirectory, SelectionDirectory};',
  'directoryContentsCanonical(storage, directories[index])',
  'directoryContentsCanonical(storage, SnapshotDirectory)',
  'directoryContentsCanonical(storage, StateDirectory)'
]) {
  if (sessionAudit.includes(forbidden)) failures.push(sessionAuditPath + ': redundant snapshot/state directory preflight returned: ' + forbidden);
}
const preflightPos = sessionAudit.indexOf('const uint32_t preflightStartedAtMs = millis();');
const storeBeginPos = sessionAudit.indexOf('JobSnapshotStore snapshots(storage);');
if (preflightPos < 0 || storeBeginPos < 0 || preflightPos >= storeBeginPos) {
  failures.push(sessionAuditPath + ': selection directory preflight must finish before mutable selection store begin path');
}

for (const text of [
  'windingSessionMetrics.directoryPreflightDurationMs', 'windingSessionMetrics.directoryPreflightMeasured',
  'WindingSessionPersistenceAuditFailure::DirectoryUnavailable', 'WindingSessionPersistenceAuditFailure::TemporaryFilePresent',
  'WindingSessionPersistenceAuditFailure::InvalidDirectoryEntry', 'session_directory_unavailable',
  'session_temp_present', 'session_directory_invalid'
]) requireText(backupExportPath, backupExport, text, 'backup manifest no longer consumes authoritative session preflight evidence: ' + text);
if (backupExport.includes('scanSessionDirectory(storage, directories[i], 0UL,')) {
  failures.push(backupExportPath + ': duplicate backup-manifest session directory preflight scan detected');
}

for (const text of [
  '"/api/warehouse/write-offs", HTTP_POST', 'm_server.send(410', 'legacy_writeoff_post_disabled',
  'write_performed', 'replacement', '/api/material-requests/warehouse',
  '"/api/warehouse/write-offs", HTTP_GET', 'handleListWriteOffs()'
]) requireText(writeOffPath, writeOff, text, 'legacy writeoff hard-deprecation contract missing: ' + text);
if (writeOff.includes('handleConfirmWriteOff')) failures.push(writeOffPath + ': retired public writeoff mutation handler returned');

for (const text of [
  'requestedMovement.sourceSessionId == 0UL', 'requestedMovement.sourceRunId == 0UL',
  'JobSpoolSelectionStore::loadReadOnly', 'selection.spoolId != spoolId',
  'WindingSessionCompletionAudit::check', 'm_warehouse.confirmedWriteOffForSourceRun',
  'alreadyConfirmed', 'm_pending.save(pending)'
]) requireText(runWirePath, runWire, text, 'atomic RUN_WIRE provenance guard missing: ' + text);
for (const forbidden of ['confirmSpoolWriteOff(', 'confirmKgFirstWriteOff(', 'SpoolWriteOffResult']) {
  if (writeOffStore.includes(forbidden) || warehouseHeader.includes(forbidden)) {
    failures.push(writeOffStorePath + ': dead legacy direct writeoff surface returned: ' + forbidden);
  }
}
for (const text of [
  'pending.mode == WarehouseWriteOffMode::LegacySpool', 'ConfirmedSpoolWriteOff operation;',
  'currentWeight == pending.weightBeforeGrams', 'currentWeight == pending.weightAfterGrams',
  'appendWriteOffRecord(pending.movementId'
]) requireText(writeOffRecoveryPath, writeOffRecovery, text, 'historical legacy recovery guard missing: ' + text);

for (const variant of ['desktop', 'mobile']) {
  for (const page of ['settings.html', 'repairs.html', 'motors.html', 'warehouse.html', 'winding-history.html', 'backup.html']) {
    requireFile('firmware/esp32/web/' + variant + '/' + page, 'required final acceptance UI page missing');
  }
}
requireText(staticSitePath, staticSite, '/shared/settings-system-diagnostics.js', 'settings diagnostics helper injection missing');
requireText(webAuditPath, webAudit, '/api/system/storage', 'web audit no longer protects microSD diagnostics');

for (const contract of [
  'physical START button polling contract missing', 'direct SSR authority detected on ESP32',
  'atomic exact-spool/source-run writeoff guard missing', 'explicit APPLY confirmation contract missing'
]) requireText(releaseAuditPath, releaseAudit, contract, 'release safety audit lost required contract: ' + contract);

if (failures.length) {
  console.error(failures.join('\n'));
  process.exit(1);
}
console.log('Final acceptance contracts OK: bounded workshop reads, selection-only preflight before recoverable spool-store begin, exact linked spool identity, diagnostics/network/storage, fail-closed backup/restore, atomic manual exact-run RUN_WIRE, removed dead direct writeoff entrypoints, deterministic historical recovery, and desktop/mobile acceptance UI.');
