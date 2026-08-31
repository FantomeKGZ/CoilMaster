const fs = require('fs');

function read(path) {
  return fs.readFileSync(path, 'utf8');
}

function requireText(source, text, label) {
  if (!source.includes(text)) throw new Error(`Missing ${label}: ${text}`);
}

function forbidText(source, text, label) {
  if (source.includes(text)) throw new Error(`Forbidden ${label}: ${text}`);
}

const desktopBackup = read('firmware/esp32/web/desktop/backup.html');
const mobileBackup = read('firmware/esp32/web/mobile/backup.html');
const staticServer = read('firmware/esp32/src/CM_StaticSiteServer.cpp');
const restoreUi = read('firmware/esp32/web/shared/backup-remote-upload.js');
const staleGuard = read('firmware/esp32/web/shared/backup-restore-stale-guard.js');
const backend = read('firmware/esp32/src/CM_RemoteBackupWeb.cpp');
const desktopSettings = read('firmware/esp32/web/desktop/settings-ftp.html');
const mobileSettings = read('firmware/esp32/web/mobile/settings-ftp.html');
const settingsUi = read('firmware/esp32/web/shared/settings-remote-backup.js');

for (const [name, page] of [['desktop', desktopBackup], ['mobile', mobileBackup]]) {
  requireText(page, '/shared/backup-restore-stale-guard.js', `${name} stale restore guard`);
  forbidText(page, '/shared/backup-remote-upload.js', `${name} static restore helper`);
}

requireText(staticServer, "if(rest==='/backup.html')", 'live backup page injection gate');
requireText(staticServer, "helper.src='/shared/backup-remote-upload.js';", 'live restore helper injection');
requireText(staleGuard, 'function hasRestoreUi()', 'delayed restore UI presence gate');
requireText(staleGuard, 'if(!hasRestoreUi())return;', 'read-only page polling suppression');

const routes = [
  ['/api/backup/remote/upload', 'HTTP_POST'],
  ['/api/backup/remote/status', 'HTTP_GET'],
  ['/api/backup/remote/batch', 'HTTP_POST'],
  ['/api/backup/remote/retention', 'HTTP_POST'],
  ['/api/backup/remote/batch-status', 'HTTP_GET'],
  ['/api/backup/remote/inspection', 'HTTP_POST'],
  ['/api/backup/remote/inspection-status', 'HTTP_GET'],
  ['/api/backup/remote/staging', 'HTTP_POST'],
  ['/api/backup/remote/staging-status', 'HTTP_GET'],
  ['/api/backup/remote/restore-plan', 'HTTP_POST'],
  ['/api/backup/remote/restore-plan-status', 'HTTP_GET'],
  ['/api/backup/remote/rollback-snapshot', 'HTTP_POST'],
  ['/api/backup/remote/rollback-snapshot-status', 'HTTP_GET'],
  ['/api/backup/remote/apply-preflight', 'HTTP_POST'],
  ['/api/backup/remote/apply-preflight-status', 'HTTP_GET'],
  ['/api/backup/remote/apply', 'HTTP_POST'],
  ['/api/backup/remote/apply-status', 'HTTP_GET']
];

for (const [route, method] of routes) {
  requireText(restoreUi, route, `restore UI route ${route}`);
  requireText(backend, `m_server.on(\"${route}\", ${method}`, `backend route ${method} ${route}`);
}
requireText(restoreUi, "fetch('/api/backup/remote/staging',{method:'DELETE'})", 'restore staging discard action');
requireText(backend, 'm_server.on("/api/backup/remote/staging", HTTP_DELETE', 'backend staging discard route');
requireText(restoreUi, "new URLSearchParams({batch_id:batchId,confirmed:'APPLY'})", 'typed apply confirmation payload');
requireText(backend, 'm_server.arg("confirmed") != F("APPLY")', 'backend apply confirmation enforcement');

for (const [name, page] of [['desktop', desktopSettings], ['mobile', mobileSettings]]) {
  for (const id of ['host', 'port', 'directory', 'username', 'password', 'retention', 'enabled', 'test', 'ftpStart', 'ftpStop']) {
    requireText(page, `id=\"${id}\"`, `${name} settings control ${id}`);
  }
  requireText(page, '/shared/settings-remote-backup.js', `${name} shared settings controller`);
  requireText(page, `CMRemoteBackupPage.start('${name}')`, `${name} shared controller startup`);
}

for (const field of [
  'enabled', 'host', 'port', 'username', 'password', 'remote_directory',
  'retention_count', 'schedule_enabled', 'schedule_hour', 'schedule_minute'
]) {
  requireText(settingsUi, field, `settings UI field ${field}`);
}
requireText(settingsUi, "fetch('/api/backup/remote/configuration'", 'settings configuration endpoint');
requireText(settingsUi, "fetch('/api/backup/remote/test',{method:'POST'})", 'settings test endpoint');
requireText(backend, 'm_server.on("/api/backup/remote/configuration", HTTP_GET', 'backend configuration GET');
requireText(backend, 'm_server.on("/api/backup/remote/configuration", HTTP_POST', 'backend configuration POST');
requireText(backend, 'm_server.on("/api/backup/remote/test", HTTP_POST', 'backend FTP test POST');
requireText(backend, 'parseCanonicalUnsigned(m_server.arg("retention_count"), 1UL, 30UL', 'retention range 1..30');
requireText(backend, 'settings.password = previous.password;', 'empty password preserves stored credential');
requireText(backend, '\"credentials_exposed\":false', 'credentials remain hidden');
requireText(backend, 'settings.scheduleEnabled = configured ? previous.scheduleEnabled : false;', 'schedule preservation/default');

console.log('Remote backup UI parity contracts OK');
