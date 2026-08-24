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
const clientHeader = read('firmware/esp32/src/CM_HardwareControlClient.h');
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
mustContain(shared, 'WAITING_LOCAL_CONFIRM', 'shared calibration controller');
mustContain(shared, 'Подтвердите калибровку клавишей # на Arduino', 'shared calibration controller');
mustContain(shared, 'физическую START', 'shared calibration controller');
mustContain(shared, 'result_available', 'shared calibration controller');
mustContain(shared, 'recommendation_valid', 'shared calibration controller');
mustContain(shared, 'recommended_direction', 'shared calibration controller');
mustNotContain(shared, '/start', 'shared calibration controller');
mustNotContain(shared, '/ssr', 'shared calibration controller');

mustContain(webCpp, '/api/hardware/hall/calibration', 'ESP32 Hall web API');
mustContain(webCpp, '/api/hardware/hall/calibration/arm', 'ESP32 Hall web API');
mustContain(webCpp, '/api/hardware/hall/calibration/abort', 'ESP32 Hall web API');
mustContain(webCpp, 'HallCalibrationRemoteState::WaitingLocalConfirm', 'ESP32 Hall web API');
mustContain(webCpp, 'HallCalibrationAnalyzer::analyzeSummary', 'ESP32 Hall web API');
mustContain(webCpp, 'result_available', 'ESP32 Hall web API');
mustContain(webCpp, 'recommendation_valid', 'ESP32 Hall web API');
mustContain(webCpp, 'recommended_direction', 'ESP32 Hall web API');
mustNotContain(webCpp, '/api/hardware/hall/calibration/start', 'ESP32 Hall web API');

mustContain(analyzerHeader, 'analyzeSummary', 'ESP32 Hall analyzer');
mustContain(analyzerCpp, 'MinimumSignalSpan', 'ESP32 Hall analyzer');
mustContain(analyzerCpp, 'recommendedThreshold', 'ESP32 Hall analyzer');
mustContain(analyzerCpp, 'recommendedHysteresis', 'ESP32 Hall analyzer');

mustContain(clientHeader, 'CalibrationKeepAliveMs = 1000UL', 'ESP32 Hall client');
mustContain(clientHeader, 'proposeHallCalibration', 'ESP32 Hall client');
mustContain(clientHeader, 'm_pendingCalibrationMeasurementId', 'ESP32 Hall client');
mustContain(clientCpp, 'CMP1|CAL|ARM|C', 'ESP32 Hall client');
mustContain(clientCpp, 'CMP1|CAL|ABORT|C', 'ESP32 Hall client');
mustContain(clientCpp, 'CMP1|CAL|GET|C', 'ESP32 Hall client');
mustContain(clientCpp, 'CMP1|CAL_PROPOSAL|%lu|%u|%u|%u|%s|C', 'ESP32 Hall client');
mustContain(clientCpp, 'CMP1|CAL_APPLIED|', 'ESP32 Hall client');
mustContain(clientCpp, 'measurementId != m_pendingCalibrationMeasurementId', 'ESP32 Hall client');
mustContain(clientCpp, 'sendCalibrationKeepAlive', 'ESP32 Hall client');
mustContain(clientCpp, 'WAITING_LOCAL_CONFIRM', 'ESP32 Hall client');
mustContain(clientCpp, 'WAITING_APPLY_CONFIRM', 'ESP32 Hall client');
mustContain(clientCpp, 'HallCalibrationRemoteState::WaitingLocalConfirm', 'ESP32 Hall client');
mustContain(clientCpp, 'CMP1|CAL_STATE|', 'ESP32 Hall client');
mustContain(clientCpp, 'CMP1|CAL_RESULT|', 'ESP32 Hall client');

mustContain(arduinoMain, 'HallCalibrationState::WaitingLocalConfirm', 'Arduino runtime');
mustContain(arduinoMain, "key == '#'", 'Arduino runtime');
mustContain(arduinoMain, 'confirmLocal', 'Arduino runtime');
mustContain(arduinoMain, 'local_confirm=ACCEPTED', 'Arduino runtime');
mustContain(arduinoMain, 'physical_start=WAITING_LOCAL_CONFIRM', 'Arduino runtime');
mustContain(arduinoMain, 'HallCalibrationState::ArmedWaitingPhysicalStart', 'Arduino runtime');
mustContain(arduinoMain, 'physicalStart', 'Arduino runtime');
mustContain(arduinoMain, 'HallCalibrationState::Running', 'Arduino runtime');
mustContain(arduinoMain, 'abort=KEYPAD_INPUT', 'Arduino runtime');
mustContain(arduinoMain, 'abort=PHYSICAL_START_PRESSED_AGAIN', 'Arduino runtime');
mustContain(arduinoMain, 'hallCalibration.notePeerContact(nowMs);', 'Arduino runtime');

mustContain(calibrationHeader, 'WaitingLocalConfirm', 'Arduino Hall calibration service');
mustContain(calibrationHeader, 'WaitingApplyConfirm', 'Arduino Hall calibration service');
mustContain(calibrationHeader, 'PeerTimeoutMs = 3000UL', 'Arduino Hall calibration service');
mustContain(calibrationHeader, 'ApplyConfirmTimeoutMs = 30000UL', 'Arduino Hall calibration service');
mustContain(calibrationHeader, 'notePeerContact', 'Arduino Hall calibration service');
mustContain(calibrationHeader, 'confirmLocal', 'Arduino Hall calibration service');
mustContain(calibrationHeader, 'beginApplyConfirm', 'Arduino Hall calibration service');
mustContain(calibrationHeader, 'measurementId', 'Arduino Hall calibration service');
mustContain(calibrationHeader, 'ArmedTimeoutMs = 60000UL', 'Arduino Hall calibration service');
mustContain(calibrationCpp, 'm_state = HallCalibrationState::WaitingLocalConfirm', 'Arduino Hall calibration service');
mustContain(calibrationCpp, 'm_state = HallCalibrationState::ArmedWaitingPhysicalStart', 'Arduino Hall calibration service');
mustContain(calibrationCpp, 'm_state = HallCalibrationState::WaitingApplyConfirm', 'Arduino Hall calibration service');
mustContain(calibrationCpp, 'result.measurementId = measurementIdentity(result)', 'Arduino Hall calibration service');
mustContain(calibrationCpp, 'nowMs - m_lastPeerContactMs', 'Arduino Hall calibration service');
mustContain(calibrationCpp, '>= PeerTimeoutMs', 'Arduino Hall calibration service');
mustContain(calibrationCpp, 'nowMs - m_armedAtMs', 'Arduino Hall calibration service');
mustContain(calibrationCpp, '>= ArmedTimeoutMs', 'Arduino Hall calibration service');
mustContain(calibrationCpp, 'result.baselineAdc', 'Arduino Hall calibration service');
mustContain(calibrationCpp, 'result.minAdc', 'Arduino Hall calibration service');
mustContain(calibrationCpp, 'result.maxAdc', 'Arduino Hall calibration service');
mustContain(calibrationCpp, 'm_resultPending', 'Arduino Hall calibration service');
mustNotContain(calibrationCpp, 'm_result.', 'Arduino Hall calibration service');
mustNotContain(calibrationCpp, 'recommendedThreshold', 'Arduino Hall calibration service');
mustNotContain(calibrationCpp, 'recommendedHysteresis', 'Arduino Hall calibration service');
mustContain(calibrationProtocolCpp, 'WAITING_LOCAL_CONFIRM', 'Arduino Hall calibration protocol');
mustContain(calibrationProtocolCpp, 'WAITING_APPLY_CONFIRM', 'Arduino Hall calibration protocol');
mustContain(calibrationProtocolCpp, 'CAL_PROPOSAL', 'Arduino Hall calibration protocol');
mustContain(calibrationProtocolCpp, 'CAL_APPLIED', 'Arduino Hall calibration protocol');
mustContain(calibrationProtocolCpp, 'CMP1|CAL_RESULT|INVALID|%u|%u|%u|0|0|RISING|%u|%lu|%lu|C', 'Arduino Hall calibration protocol');

const waitingBranch = calibrationCpp.indexOf('m_state == HallCalibrationState::WaitingLocalConfirm ||');
const armedBranch = calibrationCpp.indexOf('m_state == HallCalibrationState::ArmedWaitingPhysicalStart', waitingBranch);
const timeoutCheck = calibrationCpp.indexOf('>= ArmedTimeoutMs', waitingBranch);
const baselineCall = calibrationCpp.indexOf('sampleBaseline(nowMs);', timeoutCheck);
if (waitingBranch < 0 || armedBranch < 0 || timeoutCheck < 0 || baselineCall < 0 ||
    !(waitingBranch < armedBranch && armedBranch < timeoutCheck && timeoutCheck < baselineCall)) {
  throw new Error('Arduino Hall calibration service: baseline sampling must remain after local confirmation and armed timeout guard');
}

const peerTimeout = calibrationCpp.indexOf('>= PeerTimeoutMs');
const peerAbort = calibrationCpp.indexOf('abort();', peerTimeout);
if (peerTimeout < 0 || peerAbort < 0 || peerAbort < peerTimeout) {
  throw new Error('Arduino Hall calibration service: UART peer timeout must fail closed to abort');
}

const localConfirmBranch = arduinoMain.indexOf('HallCalibrationState::WaitingLocalConfirm');
const confirmKey = arduinoMain.indexOf("key == '#'", localConfirmBranch);
const confirmCall = arduinoMain.indexOf('hallCalibration.confirmLocal', confirmKey);
const physicalStartBranch = arduinoMain.indexOf('void processExternalStart');
const physicalStartCall = arduinoMain.indexOf('hallCalibration.physicalStart', physicalStartBranch);
if (localConfirmBranch < 0 || confirmKey < 0 || confirmCall < 0 || physicalStartBranch < 0 || physicalStartCall < 0 ||
    !(localConfirmBranch < confirmKey && confirmKey < confirmCall && confirmCall < physicalStartBranch && physicalStartBranch < physicalStartCall)) {
  throw new Error('Arduino Hall calibration runtime: required order is CAL_ARM -> local # -> separate physical START');
}

console.log('Hall calibration web/runtime contracts: OK');
