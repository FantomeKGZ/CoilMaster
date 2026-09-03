const fs = require('fs');
const path = require('path');

const root = path.resolve(__dirname, '../..');
const read = relative => fs.readFileSync(path.join(root, relative), 'utf8');
const desktop = read('firmware/esp32/web/desktop/settings.html');
const mobile = read('firmware/esp32/web/mobile/settings.html');
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
    "max_target_strands"
  ]) must(source, token, label);

  must(source, "method:'POST'", `${label} explicit settings mutation`);
  must(source, "cache:'no-store'", `${label} fresh settings status`);
}

must(desktop, "localStorage.setItem('cm-ui-version','mobile')", 'desktop mobile switch');
must(mobile, "localStorage.setItem('cm-ui-version','desktop')", 'mobile desktop switch');

if (failures.length) {
  console.error(failures.join('\n'));
  process.exit(1);
}

console.log('Settings hub parity contract OK: desktop/mobile expose the same system/data destinations, network capability status, warehouse price and calculator settings controls.');
