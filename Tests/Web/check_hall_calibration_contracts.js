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
const analyzerHeader = read('firmware/esp32/src/CM_HallCalibrationAnalyzer.h');
const analyzerCpp = read('firmware/esp32/src/CM_HallCalibrationAnalyzer.cpp');
const arduinoMain = read('firmware/arduino/src/main.cpp');
const calibrationHeader = read('Arduino/CM_HallCalibrationService.h');
const calibrationCpp = read('Arduino/CM_HallCalibrationService.cpp');
const calibrationProtocolCpp = read('Arduino/CM_HallCalibrationProtocol.cpp');

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
mustContain(webCpp, 'HallCalibrationAnalyzer::analyzeSummary', 'ESP32 Hall web API');
mustContain(webCpp, 'result_available', 'ESP32 Hall web API');
mustContain(webCpp, 'recommendation_valid', 'ESP32 Hall web API');
mustContain(webCpp, 'recommended_direction', 'ESP32 Hall web API');
mustNotContain(webCpp, '/api/hardware/hall/calibration/start', 'ESP32 Hall web API');

mustContain(analyzerHeader, 'analyzeSummary', 'ESP32 Hall analyzer');
mustContain(analyzerCpp, 'MinimumSignalSpan', 'ESP32 Hall analyzer');
mustContain(analyzerCpp, 'recommendedThreshold', 'ESP32 Hall analyzer');
mustContain(analyzerCpp, 'recommendedHysteresis', 'ESP32 Hall analyzer');

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

mustContain(calibrationHeader, 'ArmedTimeoutMs = 60000UL', 'Arduino Hall calibration service');
mustContain(calibrationCpp, 'nowMs - m_armedAtMs', 'Arduino Hall calibration service');
mustContain(calibrationCpp, '>= ArmedTimeoutMs', 'Arduino Hall calibration service');
mustContain(calibrationCpp, 'result.baselineAdc', 'Arduino Hall calibration service');
mustContain(calibrationCpp, 'result.minAdc', 'Arduino Hall calibration service');
mustContain(calibrationCpp, 'result.maxAdc', 'Arduino Hall calibration service');
mustContain(calibrationCpp, 'm_resultPending', 'Arduino Hall calibration service');
mustNotContain(calibrationCpp, 'm_result.', 'Arduino Hall calibration service');
mustNotContain(calibrationCpp, 'recommendedThreshold', 'Arduino Hall calibration service');
mustNotContain(calibrationCpp, 'recommendedHysteresis', 'Arduino Hall calibration service');
mustNotContain(calibrationCpp, 'const uint16_t upward =', 'Arduino Hall calibration service');
mustNotContain(calibrationCpp, 'const uint16_t downward =', 'Arduino Hall calibration service');
mustContain(calibrationProtocolCpp, 'CMP1|CAL_RESULT|INVALID|%u|%u|%u|0|0|RISING|%u|%lu|C', 'Arduino Hall calibration protocol');

const armedBranch = calibrationCpp.indexOf('if (m_state == HallCalibrationState::ArmedWaitingPhysicalStart)');
const baselineCall = calibrationCpp.indexOf('sampleBaseline(nowMs);', armedBranch);
const timeoutCheck = calibrationCpp.indexOf('>= ArmedTimeoutMs', armedBranch);
const abortCall = calibrationCpp.indexOf('abort();', timeoutCheck);
if (armedBranch < 0 || timeoutCheck < 0 || abortCall < 0 || baselineCall < 0 ||
    !(armedBranch < timeoutCheck && timeoutCheck < abortCall && abortCall < baselineCall)) {
  throw new Error('Arduino Hall calibration service: armed timeout must abort before baseline sampling');
}

console.log('Hall calibration web/runtime contracts: OK');
