const fs = require('fs');
const path = require('path');

const root = path.resolve(__dirname, '../..');
const read = rel => fs.readFileSync(path.join(root, rel), 'utf8');
const mustContain = (text, needle, label) => {
  if (!text.includes(needle)) throw new Error(`${label}: missing ${needle}`);
};

const hallWeb = read('firmware/esp32/src/CM_HardwareControlWeb.cpp');
const historyStore = read('firmware/esp32/src/CM_HallCalibrationHistoryStore.h');
const hallUi = read('firmware/esp32/web/shared/settings-hall-calibration.js');
const desktopHome = read('firmware/esp32/web/desktop/index.html');
const mobileMore = read('firmware/esp32/web/mobile/more.html');
const appShell = read('firmware/esp32/web/shared/app-shell.js');

mustContain(historyStore, 'MaxEntries = 10U', 'Hall history store');
mustContain(hallWeb, '/api/hardware/hall/calibration/history', 'Hall history API');
mustContain(hallWeb, 'for (int index = static_cast<int>(count) - 1; index >= 0; --index)', 'Hall history newest-first API');
mustContain(hallWeb, 'persisted_valid', 'Hall history authoritative profile API');
mustContain(hallUi, "const HISTORY_URL=CAL_URL+'/history'", 'Hall history UI');
mustContain(hallUi, 'Последние калибровки', 'Hall history UI');
mustContain(hallUi, 'Рекомендация ESP32', 'Hall history UI');
mustContain(hallUi, 'Сохранено Arduino EEPROM', 'Hall history UI');
mustContain(desktopHome, 'href="/sites/reference/desktop/"', 'desktop SD reference navigation');
mustContain(mobileMore, 'href="/sites/reference/mobile/"', 'mobile SD reference navigation');
mustContain(appShell, '`/sites/reference/${uiMode}/`', 'shared SD reference navigation');

console.log('Hall history and SD reference navigation contracts: OK');
