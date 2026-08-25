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
const receiverHeader = read('firmware/esp32/src/CM_UartEventReceiver.h');
const receiverCpp = read('firmware/esp32/src/CM_UartEventReceiver.cpp');
const analyzerHeader = read('firmware/esp32/src/CM_HallCalibrationAnalyzer.h');
const analyzerCpp = read('firmware/esp32/src/CM_HallCalibrationAnalyzer.cpp');
const arduinoMain = read('firmware/arduino/src/main.cpp');
const uartCpp = read('Arduino/CM_UartEventTransport.cpp');
const hardwareControlCpp = read('Arduino/CM_HardwareControlProtocol.cpp');
const hardwareSettingsHeader = read('Arduino/CM_HardwareSettings.h');
const calibrationHeader = read('Arduino/CM_HallCalibrationService.h');
const calibrationCpp = read('Arduino/CM_HallCalibrationService.cpp');
const calibrationProtocolCpp = read('Arduino/CM_HallCalibrationProtocol.cpp');
const calibrationDoneFormatter = read('Arduino/CM_HallCalibrationDoneFormatter.cpp');

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
mustContain(shared, "CAL_URL+'/apply'", 'shared calibration controller');
mustContain(shared, 'measurement_id', 'shared calibration controller');
mustContain(shared, 'WAITING_LOCAL_CONFIRM', 'shared calibration controller');
mustContain(shared, 'WAITING_APPLY_CONFIRM', 'shared calibration controller');
mustContain(shared, 'Подтвердите калибровку клавишей # на Arduino', 'shared calibration controller');
mustContain(shared, 'Для записи параметров в EEPROM нажмите # на Arduino', 'shared calibration controller');
mustContain(shared, 'START не подтверждает сохранение', 'shared calibration controller');
mustContain(shared, 'result_available', 'shared calibration controller');
mustContain(shared, 'recommendation_valid', 'shared calibration controller');
mustContain(shared, 'recommended_direction', 'shared calibration controller');
mustNotContain(shared, "HALL_URL+'/settings'", 'shared calibration controller');
mustNotContain(shared, '/start', 'shared calibration controller');
mustNotContain(shared, '/ssr', 'shared calibration controller');

mustContain(webCpp, '/api/hardware/hall/calibration/apply', 'ESP32 Hall web API');
mustContain(webCpp, 'handleCalibrationApply', 'ESP32 Hall web API');
mustContain(webCpp, 'm_calibrationResult.measurementId', 'ESP32 Hall web API');
mustContain(webCpp, 'm_receiver.proposeHallCalibration', 'ESP32 Hall web API');
mustContain(webCpp, 'm_calibrationResult.recommendedThreshold', 'ESP32 Hall web API');
mustContain(webCpp, 'm_calibrationResult.recommendedHysteresis', 'ESP32 Hall web API');
mustContain(webCpp, 'm_calibrationResult.direction', 'ESP32 Hall web API');
mustContain(webCpp, 'HallCalibrationRemoteState::Completed', 'ESP32 Hall web API');
mustContain(webCpp, 'HallCalibrationRemoteState::WaitingApplyConfirm', 'ESP32 Hall web API');
mustContain(webCpp, 'HallCalibrationAnalyzer::analyzeSummary', 'ESP32 Hall web API');
mustContain(webCpp, 'measurement_id', 'ESP32 Hall web API');
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
mustContain(clientCpp, 'CMP1|CFG_GET|HALL|C', 'ESP32 Hall client');
mustContain(clientCpp, 'CMP1|CAL_PROPOSAL|%lu|%u|%u|%u|%s|C', 'ESP32 Hall client');
mustContain(clientCpp, 'CMP1|CAL_APPLIED|', 'ESP32 Hall client');
mustContain(clientCpp, 'measurementId != m_pendingCalibrationMeasurementId', 'ESP32 Hall client');
mustContain(clientCpp, 'RequestType::StageCalibrationProposal', 'ESP32 Hall client');
mustContain(clientCpp, 'HallCalibrationRemoteState::WaitingApplyConfirm', 'ESP32 Hall client');
mustContain(clientCpp, 'sendCalibrationKeepAlive', 'ESP32 Hall client');
mustContain(clientCpp, 'proposalWasWaitingApply', 'ESP32 Hall client');
mustContain(clientCpp, 'finishRequest(HardwareControlReplyResult::TimedOut)', 'ESP32 Hall client');
mustContain(clientCpp, 'finishRequest(HardwareControlReplyResult::Cancelled)', 'ESP32 Hall client');
mustContain(clientCpp, 'm_pendingCalibrationMeasurementId = 0UL;', 'ESP32 Hall client');
mustContain(clientCpp, 'threshold > 1023U', 'ESP32 Hall CFG_SET fail-fast validation');
mustContain(clientCpp, 'hysteresis > 512U', 'ESP32 Hall CFG_SET fail-fast validation');
mustContain(clientCpp, 'hysteresis >= threshold', 'ESP32 Hall CFG_SET fail-fast validation');
mustContain(clientCpp, 'releaseDebounceMs > 1000U', 'ESP32 Hall CFG_SET fail-fast validation');
mustContain(clientCpp, 'if (!m_waitingReply) return true;', 'ESP32 CFG reply pending-request correlation');
mustContain(clientCpp, 'const bool directSettingsReply =', 'ESP32 CFG reply request-type correlation');
mustContain(clientCpp, 'const bool proposalEarlyReply =', 'ESP32 CFG proposal early BUSY correlation');
mustContain(clientCpp, '!categoryNack || result != HardwareControlReplyResult::Busy', 'ESP32 CFG proposal BUSY-only correlation');
mustContain(clientCpp, 'm_requestType == RequestType::GetSettings && m_waitingReply', 'ESP32 CFG_STATE sent-request correlation');
mustContain(clientCpp, 'm_requestType == RequestType::StageCalibrationProposal &&\n        m_waitingReply && m_calibrationState.valid &&', 'ESP32 proposal sent-state keepalive guard');
mustContain(clientCpp, 'm_requestType == RequestType::ArmCalibration && m_waitingReply', 'ESP32 CAL_STATE ARM sent-request correlation');
mustContain(clientCpp, 'm_requestType == RequestType::AbortCalibration && m_waitingReply', 'ESP32 CAL_STATE ABORT sent-request correlation');
mustContain(clientCpp, 'm_requestType == RequestType::GetCalibration && m_waitingReply', 'ESP32 CAL_STATE GET sent-request correlation');
mustContain(clientCpp, 'm_requestType = RequestType::GetSettings;\n            m_waitingReply = false;', 'ESP32 reconciliation must queue CFG_GET unsent');
mustContain(clientCpp, 'parsed.threshold > 1023U', 'ESP32 Hall settings state range validation');
mustContain(clientCpp, 'parsed.hysteresis > 512U', 'ESP32 Hall settings state range validation');
mustContain(clientCpp, 'parsed.hysteresis >= parsed.threshold', 'ESP32 Hall settings state range validation');
mustContain(clientCpp, 'parsed.releaseDebounceMs > 1000U', 'ESP32 Hall settings state range validation');
mustContain(clientCpp, 'expectedReleaseBoundary', 'ESP32 Hall telemetry semantic validation');
mustContain(clientCpp, 'parsed.rawAdc > 1023U', 'ESP32 Hall telemetry ADC validation');
mustContain(clientCpp, 'parsed.windowMin > 1023U', 'ESP32 Hall telemetry ADC validation');
mustContain(clientCpp, 'parsed.rawAdc < parsed.windowMin', 'ESP32 Hall telemetry window validation');
mustContain(clientCpp, 'parsed.releaseBoundary != expectedReleaseBoundary', 'ESP32 Hall telemetry release validation');
mustContain(clientCpp, 'parsed.sampleCount == 0U', 'ESP32 Hall telemetry sample validation');
mustContain(clientCpp, 'applied.threshold > 1023U', 'ESP32 Hall client applied range validation');
mustContain(clientCpp, 'applied.hysteresis > 512U', 'ESP32 Hall client applied range validation');
mustContain(clientCpp, 'applied.hysteresis >= applied.threshold', 'ESP32 Hall client applied range validation');
mustContain(clientCpp, 'applied.releaseDebounceMs > 1000U', 'ESP32 Hall client applied range validation');
mustContain(clientCpp, 'm_requestType != RequestType::StageCalibrationProposal ||\n        !m_waitingReply || measurementId != m_pendingCalibrationMeasurementId', 'ESP32 CAL_APPLIED sent-request correlation');
mustContain(clientCpp, 'applied.fromEeprom = true;', 'ESP32 Hall client');
mustContain(clientCpp, 'm_settings = applied;', 'ESP32 Hall client');

mustContain(receiverHeader, 'proposeHallCalibration', 'ESP32 UART receiver');
mustContain(receiverCpp, 'm_hardwareControl.proposeHallCalibration', 'ESP32 UART receiver');

mustContain(uartCpp, 'CMP1|CAL_PROPOSAL|', 'Arduino UART transport');
mustContain(uartCpp, 'HallCalibrationProtocol::parseProposal', 'Arduino UART transport');
mustContain(uartCpp, 'StageHallCalibrationProposal', 'Arduino UART transport');
mustContain(uartCpp, 'sendHallCalibrationApplied', 'Arduino UART transport');

mustContain(hardwareControlCpp, 'CAL_PROPOSAL', 'Arduino hardware-control protocol');
mustContain(hardwareControlCpp, 'StageHallCalibrationProposal', 'Arduino hardware-control protocol');
mustContain(hardwareControlCpp, "text[0] == '0' && text[1] != '\\0'", 'Arduino hardware-control protocol');
mustContain(calibrationProtocolCpp, 'HardwareControlProtocol::parseRequest', 'Arduino Hall calibration protocol');
mustContain(calibrationProtocolCpp, 'StageHallCalibrationProposal', 'Arduino Hall calibration protocol');
mustNotContain(calibrationProtocolCpp, 'bool parseDecimal32', 'Arduino Hall calibration protocol');
mustNotContain(calibrationProtocolCpp, 'bool parseDecimal16', 'Arduino Hall calibration protocol');

mustContain(arduinoMain, 'HallCalibrationState::WaitingLocalConfirm', 'Arduino runtime');
mustContain(arduinoMain, 'HallCalibrationState::WaitingApplyConfirm', 'Arduino runtime');
mustContain(arduinoMain, "key == '#'", 'Arduino runtime');
mustContain(arduinoMain, 'confirmLocal', 'Arduino runtime');
mustContain(arduinoMain, 'local_confirm=ACCEPTED', 'Arduino runtime');
mustContain(arduinoMain, 'physical_start=WAITING_LOCAL_CONFIRM', 'Arduino runtime');
mustContain(arduinoMain, 'physical_start=WAITING_APPLY_CONFIRM', 'Arduino runtime');
mustContain(arduinoMain, 'StageHallCalibrationProposal', 'Arduino runtime');
mustContain(arduinoMain, 'measured.measurementId != request.measurementId', 'Arduino runtime');
mustContain(arduinoMain, 'hallCalibration.beginApplyConfirm(request.measurementId, nowMs)', 'Arduino runtime');
mustContain(arduinoMain, 'hardwareSettingsController.apply(pendingHallCalibrationSettings)', 'Arduino runtime');
mustContain(arduinoMain, 'sendHallCalibrationApplied', 'Arduino runtime');
mustContain(arduinoMain, 'hardwareSettingsController.settings());', 'Arduino runtime');
mustContain(arduinoMain, 'clearPendingHallCalibrationProposal', 'Arduino runtime');
mustContain(arduinoMain, 'HallCalibrationApplyResult::Cancelled', 'Arduino runtime');
mustContain(arduinoMain, 'hallCalibration.notePeerContact(nowMs);', 'Arduino runtime');

mustNotContain(hardwareSettingsHeader, 'measurementId', 'Arduino hardware settings EEPROM schema');
mustNotContain(hardwareSettingsHeader, 'proposal', 'Arduino hardware settings EEPROM schema');
mustNotContain(hardwareSettingsHeader, 'WaitingApplyConfirm', 'Arduino hardware settings EEPROM schema');
mustContain(hardwareSettingsHeader, 'hallThreshold', 'Arduino hardware settings EEPROM schema');
mustContain(hardwareSettingsHeader, 'hallHysteresis', 'Arduino hardware settings EEPROM schema');
mustContain(hardwareSettingsHeader, 'hallReleaseDebounceMs', 'Arduino hardware settings EEPROM schema');
mustContain(hardwareSettingsHeader, 'crc', 'Arduino hardware settings EEPROM schema');

mustContain(calibrationHeader, 'WaitingLocalConfirm', 'Arduino Hall calibration service');
mustContain(calibrationHeader, 'WaitingApplyConfirm', 'Arduino Hall calibration service');
mustContain(calibrationHeader, 'PeerTimeoutMs = 3000UL', 'Arduino Hall calibration service');
mustContain(calibrationHeader, 'ApplyConfirmTimeoutMs = 30000UL', 'Arduino Hall calibration service');
mustContain(calibrationHeader, 'beginApplyConfirm', 'Arduino Hall calibration service');
mustContain(calibrationHeader, 'measurementId', 'Arduino Hall calibration service');
mustContain(calibrationHeader, 'measurementIdentity(uint32_t completedAtMs)', 'Arduino Hall calibration service');
mustContain(calibrationCpp, 'm_state = HallCalibrationState::WaitingApplyConfirm', 'Arduino Hall calibration service');
mustContain(calibrationCpp, 'm_measurementId = measurementIdentity(nowMs)', 'Arduino Hall calibration service');
mustContain(calibrationCpp, 'result.measurementId = m_measurementId', 'Arduino Hall calibration service');
mustContain(calibrationCpp, 'nowMs - m_lastPeerContactMs', 'Arduino Hall calibration service');
mustContain(calibrationCpp, '>= PeerTimeoutMs', 'Arduino Hall calibration service');
mustContain(calibrationCpp, 'ApplyConfirmTimeoutMs', 'Arduino Hall calibration service');
mustNotContain(calibrationCpp, 'recommendedThreshold', 'Arduino Hall calibration service');
mustNotContain(calibrationCpp, 'recommendedHysteresis', 'Arduino Hall calibration service');

mustContain(calibrationProtocolCpp, 'WAITING_LOCAL_CONFIRM', 'Arduino Hall calibration protocol');
mustContain(calibrationProtocolCpp, 'WAITING_APPLY_CONFIRM', 'Arduino Hall calibration protocol');
mustContain(calibrationProtocolCpp, 'CAL_APPLIED', 'Arduino Hall calibration protocol');
mustContain(calibrationProtocolCpp, '!settings.isValid()', 'Arduino Hall calibration protocol');
mustContain(calibrationProtocolCpp, 'return formatDone(result, output, outputSize);', 'Arduino Hall compact completion TX');
mustNotContain(calibrationProtocolCpp, 'CMP1|CAL_RESULT|', 'Arduino Hall legacy completion TX');
mustContain(calibrationDoneFormatter, 'CMP1|CAL_DONE|%lu|C', 'Arduino Hall compact completion protocol');
mustContain(calibrationDoneFormatter, 'Cmp1Crc::calculate', 'Arduino Hall compact completion protocol');
mustNotContain(calibrationDoneFormatter, 'StartOrResume', 'Arduino Hall compact completion safety');
mustNotContain(calibrationDoneFormatter, 'digitalWrite', 'Arduino Hall compact completion safety');

const localConfirmBranch = arduinoMain.indexOf('HallCalibrationState::WaitingLocalConfirm');
const firstConfirmKey = arduinoMain.indexOf("key == '#'", localConfirmBranch);
const physicalStartBranch = arduinoMain.indexOf('void processExternalStart');
if (localConfirmBranch < 0 || firstConfirmKey < 0 || physicalStartBranch < 0 ||
    !(localConfirmBranch < firstConfirmKey && firstConfirmKey < physicalStartBranch)) {
  throw new Error('Arduino Hall calibration runtime: CAL_ARM must require local # before physical START');
}

const applyBranch = arduinoMain.indexOf('HallCalibrationState::WaitingApplyConfirm');
const applyConfirmKey = arduinoMain.indexOf("key == '#'", applyBranch);
const eepromApply = arduinoMain.indexOf('hardwareSettingsController.apply(pendingHallCalibrationSettings)', applyConfirmKey);
if (applyBranch < 0 || applyConfirmKey < 0 || eepromApply < 0 ||
    !(applyBranch < applyConfirmKey && applyConfirmKey < eepromApply)) {
  throw new Error('Arduino Hall calibration runtime: EEPROM apply must remain behind local # confirmation');
}

const proposalCase = arduinoMain.indexOf('HardwareControlRequestType::StageHallCalibrationProposal');
const stageConfirm = arduinoMain.indexOf('hallCalibration.beginApplyConfirm', proposalCase);
const proposalApply = arduinoMain.indexOf('hardwareSettingsController.apply(', proposalCase);
if (proposalCase < 0 || stageConfirm < 0 || (proposalApply >= 0 && proposalApply < stageConfirm)) {
  throw new Error('Arduino Hall calibration runtime: proposal staging must not write EEPROM');
}

const applyWrite = arduinoMain.indexOf('hardwareSettingsController.apply(pendingHallCalibrationSettings)', applyBranch);
const appliedReply = arduinoMain.indexOf('espTransport.sendHallCalibrationApplied(', applyWrite);
const persistedProfileReply = arduinoMain.indexOf('hardwareSettingsController.settings());', appliedReply);
const clearProposal = arduinoMain.indexOf('clearPendingHallCalibrationProposal();', appliedReply);
if (applyWrite < 0 || appliedReply < 0 || persistedProfileReply < 0 || clearProposal < 0 ||
    !(applyWrite < appliedReply && appliedReply < persistedProfileReply && persistedProfileReply < clearProposal)) {
  throw new Error('Arduino Hall calibration runtime: CAL_APPLIED must report authoritative post-apply profile before transient proposal is cleared');
}

const stagedStateGuard = clientCpp.indexOf('const bool proposalWasWaitingApply');
const stagedCompleted = clientCpp.indexOf('parsed.state == HallCalibrationRemoteState::Completed', stagedStateGuard);
const stagedCfgGet = clientCpp.indexOf('CMP1|CFG_GET|HALL|C', stagedCompleted);
const stagedSwitch = clientCpp.indexOf('m_requestType = RequestType::GetSettings;', stagedCfgGet);
const settingsHandler = clientCpp.indexOf('bool HardwareControlClient::processSettingsState');
const settingsMirror = clientCpp.indexOf('m_settings = parsed;', settingsHandler);
const stagedTimeout = clientCpp.indexOf('finishRequest(HardwareControlReplyResult::TimedOut)', settingsMirror);
if (stagedStateGuard < 0 || stagedCompleted < 0 || stagedCfgGet < 0 || stagedSwitch < 0 ||
    settingsHandler < 0 || settingsMirror < 0 || stagedTimeout < 0 ||
    !(stagedStateGuard < stagedCompleted && stagedCompleted < stagedCfgGet && stagedCfgGet < stagedSwitch &&
      settingsHandler < settingsMirror && settingsMirror < stagedTimeout)) {
  throw new Error('ESP32 Hall client: completed staged proposal must read authoritative EEPROM state before ambiguous timeout completion');
}

const abortMethod = clientCpp.indexOf('bool HardwareControlClient::abortHallCalibration()');
const abortStage = clientCpp.indexOf('RequestType::StageCalibrationProposal', abortMethod);
const abortWaiting = clientCpp.indexOf('HallCalibrationRemoteState::WaitingApplyConfirm', abortStage);
const abortClear = clientCpp.indexOf('m_requestType = RequestType::None;', abortWaiting);
const abortQueue = clientCpp.indexOf('queueRequest(RequestType::AbortCalibration', abortClear);
if (abortMethod < 0 || abortStage < 0 || abortWaiting < 0 || abortClear < 0 || abortQueue < 0 ||
    !(abortMethod < abortStage && abortStage < abortWaiting && abortWaiting < abortClear && abortClear < abortQueue)) {
  throw new Error('ESP32 Hall client: Web abort must replace a staged apply-confirm request with CAL ABORT');
}

const peerTimeout = calibrationCpp.indexOf('>= PeerTimeoutMs');
const peerAbort = calibrationCpp.indexOf('abort();', peerTimeout);
if (peerTimeout < 0 || peerAbort < 0 || peerAbort < peerTimeout) {
  throw new Error('Arduino Hall calibration service: UART peer timeout must fail closed to abort');
}

const applyTimeoutState = calibrationCpp.indexOf('m_state == HallCalibrationState::WaitingApplyConfirm');
const applyTimeoutElapsed = calibrationCpp.indexOf('nowMs - m_applyConfirmAtMs', applyTimeoutState);
const applyTimeoutLimit = calibrationCpp.indexOf('ApplyConfirmTimeoutMs', applyTimeoutElapsed);
const applyTimeoutAbort = calibrationCpp.indexOf('abort();', applyTimeoutLimit);
if (applyTimeoutState < 0 || applyTimeoutElapsed < 0 || applyTimeoutLimit < 0 || applyTimeoutAbort < 0 ||
    !(applyTimeoutState < applyTimeoutElapsed && applyTimeoutElapsed < applyTimeoutLimit && applyTimeoutLimit < applyTimeoutAbort)) {
  throw new Error('Arduino Hall calibration service: apply confirmation timeout must fail closed to abort');
}

console.log('Hall calibration exact-id/local-confirm/no-replay/reconciliation contracts: OK');
