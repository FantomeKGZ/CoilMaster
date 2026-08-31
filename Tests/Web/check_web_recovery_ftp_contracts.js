const fs = require('fs');

function read(path) {
  return fs.readFileSync(path, 'utf8');
}

function requireText(source, text, label) {
  if (!source.includes(text)) {
    throw new Error(`Missing ${label}: ${text}`);
  }
}

const desktop = read('firmware/esp32/web/desktop/settings-ftp.html');
const mobile = read('firmware/esp32/web/mobile/settings-ftp.html');
const controller = read('firmware/esp32/web/shared/settings-remote-backup.js');
const ftp = read('firmware/esp32/src/CM_WebRecoveryFtpServer.cpp');
const main = read('firmware/esp32/src/main.cpp');

for (const [name, page] of [['desktop', desktop], ['mobile', mobile]]) {
  requireText(page, 'id="ftpStart"', `${name} FTP start control`);
  requireText(page, 'id="ftpStop"', `${name} FTP stop control`);
  requireText(page, 'id="device"', `${name} FTP status container`);
  requireText(page, '/shared/settings-remote-backup.js', `${name} shared FTP controller`);
  requireText(page, "CMRemoteBackupPage.start('", `${name} shared controller startup`);
  requireText(page, '<code>/web</code>', `${name} /web scope disclosure`);
}

for (const endpoint of [
  '/api/ftp/status',
  '/api/backup/remote/configuration',
  '/api/backup/remote/test'
]) {
  requireText(controller, endpoint, `shared UI endpoint ${endpoint}`);
}
requireText(controller, "fetch('/api/ftp/'+action,{method:'POST'})", 'shared FTP action endpoint');
requireText(controller, "$('ftpStart').onclick=()=>ftpAction('start')", 'shared FTP start action');
requireText(controller, "$('ftpStop').onclick=()=>ftpAction('stop')", 'shared FTP stop action');

requireText(ftp, 'constexpr char WebRoot[] = "/web";', 'FTP storage root confinement');
requireText(ftp, 'if (!activitySafe())', 'FTP fail-closed activity gate');
requireText(ftp, 'm_transferTemporaryPath = storagePath + F(".part")', 'temporary upload path');
requireText(ftp, 'if (!m_storage.rename(m_transferTemporaryPath, m_transferPath))', 'atomic upload replacement');
requireText(ftp, 'm_storage.exists("/web/index.html")', 'root entrypoint readiness');
requireText(ftp, 'm_storage.exists("/web/desktop/index.html")', 'desktop entrypoint readiness');
requireText(ftp, 'm_storage.exists("/web/mobile/index.html")', 'mobile entrypoint readiness');
requireText(ftp, 'if (m_automaticRecovery && webRootUsable())', 'automatic recovery shutdown');
requireText(ftp, 'if (!sameLocalSubnet(candidate.remoteIP()))', 'local network access gate');
requireText(ftp, 'storagePath = String(WebRoot)', 'resolved path anchored under /web');

requireText(main, 'webRecoveryFtp.setActivityProbe(backupRuntimeActivity);', 'runtime safety probe wiring');
requireText(main, 'webRecoveryFtp.begin(webRecoveryRequired);', 'FTP recovery startup wiring');
requireText(main, '!SD.exists("/web/desktop/index.html")', 'desktop missing-site recovery trigger');
requireText(main, '!SD.exists("/web/mobile/index.html")', 'mobile missing-site recovery trigger');

console.log('Web recovery FTP contracts OK');
