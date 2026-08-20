const fs = require('fs');
const path = require('path');

const root = path.resolve(__dirname, '../..');
const read = rel => fs.readFileSync(path.join(root, rel), 'utf8');
const mustContain = (text, needle, label) => {
  if (!text.includes(needle)) throw new Error(`${label}: missing ${needle}`);
};
const mustNotContain = (text, needle, label) => {
  if (text.includes(needle)) throw new Error(`${label}: forbidden ${needle}`);
};

const desktop = read('firmware/esp32/web/desktop/settings-hall.html');
const mobile = read('firmware/esp32/web/mobile/settings-hall.html');
const shared = read('firmware/esp32/web/shared/settings-hall-calibration.js');
const webCpp = read('firmware/esp32/src/CM_HardwareControlWeb.cpp');
const clientCpp = read('firmware/esp32/src/CM_HardwareControlClient.cpp');
const arduinoMain = read('firmware/arduino/src/main.cpp');

for (const [name, page] of [['desktop', desktop], ['mobile', mobile]]) {
  mustContain(page, '/shared/settings-hall-calibration.js', `${name} Hall page`);
  mustContain(page, 'calibrationArmBtn', `${name} Hall page`);
  mustContain(page, 'calibrationAbortBtn', `${name} Hall page`);
  mustContain(page, 'calibrationApplyBtn', `${name} Hall page`);
  mustContain(page, 'физическ', `${name} Hall page`);
  mustContain(page, 'START', `${name} Hall page`);
}

mustContain(shared, "'/api/hardware/hall/calibration'", 'shared calibration controller');
mustContain(shared, "CAL_URL+'/arm'", 'shared calibration controller');
mustContain(shared, "CAL_URL+'/abort'", 'shared calibration controller');
mustContain(shared, "CAL_URL+'/refresh'", 'shared calibration controller');
mustContain(shared, "HALL_URL+'/settings'", 'shared calibration controller');
mustContain(shared, 'Нажмите физическую START', 'shared calibration controller');
mustContain(shared, 'result_available', 'shared calibration controller');
mustContain(shared, 'recommendation_valid', 'shared calibration controller');
mustContain(shared, 'recommended_direction', 'shared calibration controller');
mustNotContain(shared, '/start', 'shared calibration controller');
mustNotContain(shared, '/ssr', 'shared calibration controller');

mustContain(webCpp, '/api/hardware/hall/calibration', 'ESP32 Hall web API');
mustContain(webCpp, '/api/hardware/hall/calibration/arm', 'ESP32 Hall web API');
mustContain(webCpp, '/api/hardware/hall/calibration/abort', 'ESP32 Hall web API');
mustContain(webCpp, 'result_available', 'ESP32 Hall web API');
mustContain(webCpp, 'recommendation_valid', 'ESP32 Hall web API');
mustContain(webCpp, 'recommended_direction', 'ESP32 Hall web API');
mustNotContain(webCpp, '/api/hardware/hall/calibration/start', 'ESP32 Hall web API');

mustContain(clientCpp, 'CMP1|CAL|ARM|C', 'ESP32 Hall client');
mustContain(clientCpp, 'CMP1|CAL|ABORT|C', 'ESP32 Hall client');
mustContain(clientCpp, 'CMP1|CAL|GET|C', 'ESP32 Hall client');
mustContain(clientCpp, 'CMP1|CAL_STATE|', 'ESP32 Hall client');
mustContain(clientCpp, 'CMP1|CAL_RESULT|', 'ESP32 Hall client');

mustContain(arduinoMain, 'HallCalibrationState::ArmedWaitingPhysicalStart', 'Arduino runtime');
mustContain(arduinoMain, 'physicalStart', 'Arduino runtime');
mustContain(arduinoMain, 'HallCalibrationState::Running', 'Arduino runtime');
mustContain(arduinoMain, 'abort=KEYPAD_INPUT', 'Arduino runtime');
mustContain(arduinoMain, 'abort=PHYSICAL_START_PRESSED_AGAIN', 'Arduino runtime');

console.log('Hall calibration web/runtime contracts: OK');
