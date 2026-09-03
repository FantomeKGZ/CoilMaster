const fs = require('fs');
const path = require('path');

const root = path.resolve(__dirname, '../..');
const read = relative => fs.readFileSync(path.join(root, relative), 'utf8');
const desktop = read('firmware/esp32/web/desktop/settings.html');
const mobile = read('firmware/esp32/web/mobile/settings.html');
const desktopTime = read('firmware/esp32/web/desktop/settings-time.html');
const mobileTime = read('firmware/esp32/web/mobile/settings-time.html');
const desktopHall = read('firmware/esp32/web/desktop/settings-hall.html');
const mobileHall = read('firmware/esp32/web/mobile/settings-hall.html');
const remoteBackup = read('firmware/esp32/web/shared/settings-remote-backup.js');
const desktopPricingAudit = read('firmware/esp32/web/desktop/pricing-audit.html');
const mobilePricingAudit = read('firmware/esp32/web/mobile/pricing-audit.html');
const failures = [];

const must = (source, token, label) => {
  if (!source.includes(token)) failures.push(`${label}: missing ${token}`);
};

const pairedPages = [
  'settings-wifi.html',
  'settings-time.html',
  'settings-hall.html',
  'settings-ftp.html',
  'service-job.html',
  'motor-import.html',
  'material-catalog.html',
  'backup.html',
  'pricing-audit.html',
  'winding-history.html',
  'arduino-windings.html'
];
for (const page of pairedPages) {
  must(desktop, `/desktop/${page}`, `desktop settings ${page}`);
  must(mobile, `/mobile/${page}`, `mobile settings ${page}`);
}

for (const [label, source] of [['desktop settings', desktop], ['mobile settings', mobile]]) {
  for (const token of [
    '/api/system/network',
    '/api/warehouse/price',
    '/api/calculator/settings',
    "priceForm",
    "calcForm",
    "saved_profiles_supported",
    "ftp_supported",
    "allow_mixed_diameters",
    "max_target_strands",
    "esc=v=>String(v??'').replace(/[&<>\"']/g",
    'esc(j.mode)',
    'esc(j.ap_ssid)',
    'esc(j.ap_ip)',
    'esc(j.sta_ssid)',
    'esc(j.sta_ip)'
  ]) must(source, token, label);

  must(source, "method:'POST'", `${label} explicit settings mutation`);
  must(source, "cache:'no-store'", `${label} fresh settings status`);
}

for (const [label, source] of [['desktop time settings', desktopTime], ['mobile time settings', mobileTime]]) {
  for (const token of [
    '/api/system/time',
    "esc=v=>String(v??'').replace(/[&<>\"']/g",
    "esc(j.local_time||'не задано')",
    "esc(j.timezone||'Asia/Bishkek')",
    'esc(ntpText(j.ntp_status))'
  ]) must(source, token, label);
}

for (const [label, source] of [['desktop Hall settings', desktopHall], ['mobile Hall settings', mobileHall]]) {
  for (const token of [
    '/api/hardware/hall',
    "esc=v=>String(v??'').replace(/[&<>\"']/g",
    "esc(j.source||'—')",
    "esc(j.last_reply||'NONE')"
  ]) must(source, token, label);
}

for (const token of [
  '/api/ftp/status',
  "const esc=value=>String(value??'').replace(/[&<>\"']/g",
  "addresses.map(a=>esc(a)+':'+esc(f.port))",
  'esc(ftpResultText(f.last_result))'
]) must(remoteBackup, token, 'shared FTP settings runtime escaping');

for (const [label, source] of [['desktop pricing audit', desktopPricingAudit], ['mobile pricing audit', mobilePricingAudit]]) {
  for (const token of [
    '/api/repairs/pricing-history?repair_id=',
    "esc=v=>String(v??'').replace(/[&<>\"']/g",
    "esc(c.pricing_status||'—')",
    'esc(dateLabel(c.pricing_updated_at))'
  ]) must(source, token, label);
}

must(desktop, 'esc(j.sta_rssi)', 'desktop settings STA RSSI escaping');
must(desktop, "localStorage.setItem('cm-ui-version','mobile')", 'desktop mobile switch');
must(mobile, "localStorage.setItem('cm-ui-version','desktop')", 'mobile desktop switch');

if (failures.length) {
  console.error(failures.join('\n'));
  process.exit(1);
}

console.log('Settings/completeness contract OK: desktop/mobile expose the same settings destinations and controls, and runtime network/time/Hall/FTP/pricing-audit strings are HTML-escaped before innerHTML rendering.');
