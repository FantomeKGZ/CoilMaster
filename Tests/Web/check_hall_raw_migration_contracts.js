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

const collectorH = read('firmware/esp32/src/CM_HallCalibrationRawCollector.h');
const collectorCpp = read('firmware/esp32/src/CM_HallCalibrationRawCollector.cpp');
const rawProtocolH = read('firmware/esp32/src/CM_HallCalibrationRawProtocol.h');
const rawProtocolCpp = read('firmware/esp32/src/CM_HallCalibrationRawProtocol.cpp');
const doneProtocolH = read('firmware/esp32/src/CM_HallCalibrationDoneProtocol.h');
const doneProtocolCpp = read('firmware/esp32/src/CM_HallCalibrationDoneProtocol.cpp');
const completionAdapterH = read('firmware/esp32/src/CM_HallCalibrationCompletionAdapter.h');
const completionAdapterCpp = read('firmware/esp32/src/CM_HallCalibrationCompletionAdapter.cpp');
const receiverH = read('firmware/esp32/src/CM_UartEventReceiver.h');
const receiverCpp = read('firmware/esp32/src/CM_UartEventReceiver.cpp');
const unoProtocolH = read('Arduino/CM_HallCalibrationProtocol.h');
const unoProtocolCpp = read('Arduino/CM_HallCalibrationProtocol.cpp');
const unoDoneFormatter = read('Arduino/CM_HallCalibrationDoneFormatter.cpp');
const unoBridgeH = read('Arduino/CM_HallCalibrationRawBridge.h');
const unoBridgeCpp = read('Arduino/CM_HallCalibrationRawBridge.cpp');
const unoTransportH = read('Arduino/CM_UartEventTransport.h');
const analyzerCpp = read('firmware/esp32/src/CM_HallCalibrationAnalyzer.cpp');
const unoServiceH = read('Arduino/CM_HallCalibrationService.h');
const unoService = read('Arduino/CM_HallCalibrationService.cpp');
const unoMain = read('firmware/arduino/src/main.cpp');
const handoff = read('docs/PROJECT_HANDOFF/71_HALL_RAW_STREAM_MIGRATION_2026-08-24.md');

mustContain(collectorH, 'class HallCalibrationRawCollector', 'ESP32 raw collector');
mustContain(collectorH, 'addBaselineSample', 'ESP32 raw collector');
mustContain(collectorH, 'addRunSample', 'ESP32 raw collector');
mustContain(collectorH, 'HallCalibrationRawSummary', 'ESP32 raw collector');
mustContain(collectorCpp, 'm_baselineSum', 'ESP32 raw collector');
mustContain(collectorCpp, 'm_minAdc', 'ESP32 raw collector');
mustContain(collectorCpp, 'm_maxAdc', 'ESP32 raw collector');
mustContain(collectorCpp, 'MinimumBaselineSamples', 'ESP32 raw collector');

mustContain(rawProtocolH, 'HallCalibrationRawSample', 'ESP32 raw protocol');
mustContain(rawProtocolH, 'parseSample', 'ESP32 raw protocol');
mustContain(rawProtocolCpp, 'strcmp(version, "CMP1")', 'ESP32 raw protocol');
mustContain(rawProtocolCpp, 'strcmp(category, "CAL_SAMPLE")', 'ESP32 raw protocol');
mustContain(rawProtocolCpp, 'BASELINE', 'ESP32 raw protocol');
mustContain(rawProtocolCpp, 'RUN', 'ESP32 raw protocol');
mustContain(rawProtocolCpp, 'Cmp1Crc::calculate', 'ESP32 raw protocol');
mustContain(rawProtocolCpp, '1023UL', 'ESP32 raw protocol');
mustContain(rawProtocolCpp, '65535UL', 'ESP32 raw protocol');

mustContain(doneProtocolH, 'HallCalibrationDone', 'ESP32 done protocol');
mustContain(doneProtocolH, 'parseDone', 'ESP32 done protocol');
mustContain(doneProtocolCpp, 'strcmp(category, "CAL_DONE")', 'ESP32 done protocol');
mustContain(doneProtocolCpp, 'Cmp1Crc::calculate', 'ESP32 done protocol');
mustContain(doneProtocolCpp, 'done.measurementId == 0UL', 'ESP32 done protocol');
mustContain(doneProtocolCpp, 'strcmp(capability, "C")', 'ESP32 done protocol');
mustNotContain(doneProtocolCpp, 'StartOrResume', 'ESP32 done protocol safety');
mustNotContain(doneProtocolCpp, 'digitalWrite', 'ESP32 done protocol safety');

mustContain(completionAdapterH, 'buildFromDone', 'ESP32 completion adapter');
mustContain(completionAdapterH, 'enrichLegacy', 'ESP32 completion adapter');
mustContain(completionAdapterCpp, 'collector.finish(rawDurationMs)', 'ESP32 completion adapter');
mustContain(completionAdapterCpp, 'result.measurementId = done.measurementId', 'ESP32 completion adapter');
mustContain(completionAdapterCpp, 'result.baselineAdc = summary.baselineAdc', 'ESP32 completion adapter');
mustContain(completionAdapterCpp, 'result.minAdc = summary.minAdc', 'ESP32 completion adapter');
mustContain(completionAdapterCpp, 'result.maxAdc = summary.maxAdc', 'ESP32 completion adapter');
mustContain(completionAdapterCpp, 'result.sampleCount = summary.runSamples', 'ESP32 completion adapter');
mustContain(completionAdapterCpp, 'result.durationMs = summary.durationMs', 'ESP32 completion adapter');
mustNotContain(completionAdapterCpp, 'StartOrResume', 'ESP32 completion adapter safety');
mustNotContain(completionAdapterCpp, 'digitalWrite', 'ESP32 completion adapter safety');

mustContain(receiverH, 'HallCalibrationRawCollector m_hallCalibrationRaw', 'ESP32 raw receiver');
mustContain(receiverH, 'processHallCalibrationDone', 'ESP32 compact completion receiver state');
mustContain(receiverH, 'm_compactCalibrationResult', 'ESP32 compact completion receiver state');
mustContain(receiverH, 'm_hasCompactCalibrationResult', 'ESP32 compact completion receiver state');
mustContain(receiverCpp, 'CMP1|CAL_SAMPLE|', 'ESP32 raw receiver');
mustContain(receiverCpp, 'CMP1|CAL_DONE|', 'ESP32 compact completion dispatch');
mustContain(receiverCpp, 'processHallCalibrationDone(m_line, millis())', 'ESP32 compact completion dispatch');
mustContain(receiverCpp, 'HallCalibrationRawProtocol::parseSample', 'ESP32 raw receiver');
mustContain(receiverCpp, 'm_hallCalibrationRaw.addBaselineSample', 'ESP32 raw receiver');
mustContain(receiverCpp, 'm_hallCalibrationRaw.addRunSample', 'ESP32 raw receiver');
mustContain(receiverCpp, 'HallCalibrationDoneProtocol::parseDone', 'ESP32 compact completion parser hook');
mustContain(receiverCpp, 'HallCalibrationCompletionAdapter::buildFromDone', 'ESP32 compact completion result path');
mustContain(receiverCpp, 'HallCalibrationCompletionAdapter::enrichLegacy', 'ESP32 legacy completion result path');
mustContain(receiverCpp, 'm_hardwareControl.takeHallCalibrationResult(result)', 'ESP32 legacy completion fallback');

mustContain(unoProtocolH, 'HallCalibrationSamplePhase', 'Uno raw protocol');
mustContain(unoProtocolH, 'formatSample', 'Uno raw protocol');
mustContain(unoProtocolH, 'formatDone', 'Uno compact completion protocol');
mustContain(unoProtocolCpp, 'CMP1|CAL_SAMPLE|%S|%u|%u|%lu|C', 'Uno raw protocol');
mustContain(unoProtocolCpp, 'PSTR("BASELINE")', 'Uno raw protocol');
mustContain(unoProtocolCpp, 'PSTR("RUN")', 'Uno raw protocol');
mustContain(unoProtocolCpp, 'rawAdc > 1023U', 'Uno raw protocol');
mustContain(unoProtocolCpp, 'appendCrc(output, outputSize, length)', 'Uno raw protocol');
mustContain(unoProtocolCpp, 'return formatDone(result, output, outputSize);', 'Uno active compact completion TX');
mustNotContain(unoProtocolCpp, 'CMP1|CAL_RESULT|', 'Uno legacy completion TX removed');
mustContain(unoDoneFormatter, 'CMP1|CAL_DONE|%lu|C', 'Uno compact completion formatter');
mustContain(unoDoneFormatter, 'result.measurementId == 0UL', 'Uno compact completion formatter');
mustContain(unoDoneFormatter, 'CM_CrcFrameText.h', 'Uno compact completion formatter');
mustContain(unoDoneFormatter, 'CrcFrameText::append', 'Uno compact completion formatter');
mustNotContain(unoDoneFormatter, 'StartOrResume', 'Uno compact completion formatter safety');
mustNotContain(unoDoneFormatter, 'digitalWrite', 'Uno compact completion formatter safety');

mustContain(unoBridgeH, 'no START', 'Uno raw bridge safety');
mustContain(unoBridgeCpp, 'sendHallCalibrationSample', 'Uno raw bridge');
mustContain(unoTransportH, 'sendHallCalibrationSample', 'Uno UART raw TX');
mustContain(unoService, 'HallCalibrationRawBridge::publish', 'Uno raw sample publisher');
mustContain(unoService, 'HallCalibrationSamplePhase::Baseline', 'Uno baseline raw publisher');
mustContain(unoService, 'HallCalibrationSamplePhase::Run', 'Uno run raw publisher');

// Extended measurement aggregation now belongs exclusively to ESP32.
mustNotContain(unoServiceH, 'm_baselineSum', 'Uno calibration service');
mustNotContain(unoServiceH, 'm_minAdc', 'Uno calibration service');
mustNotContain(unoServiceH, 'm_maxAdc', 'Uno calibration service');
mustNotContain(unoServiceH, 'm_resultDurationMs', 'Uno calibration service');
mustNotContain(unoService, 'm_baselineSum', 'Uno calibration service');
mustNotContain(unoService, 'm_minAdc', 'Uno calibration service');
mustNotContain(unoService, 'm_maxAdc', 'Uno calibration service');
mustContain(unoServiceH, 'm_measurementId', 'Uno correlation identity');
mustContain(unoService, 'result.measurementId = m_measurementId', 'Uno correlation identity');

for (const field of ['baselineAdc', 'minAdc', 'maxAdc', 'sampleCount', 'durationMs']) {
  mustNotContain(unoServiceH, field, 'Uno identity-only result');
}

mustContain(analyzerCpp, 'analyzeSummary', 'ESP32 analyzer owner');

mustContain(unoMain, 'processExternalStart', 'Uno safety runtime');
mustContain(unoMain, 'ssr.update', 'Uno SSR owner');
mustContain(unoMain, 'processTurnSource', 'Uno turn counter owner');
mustContain(unoMain, 'WaitingApplyConfirm', 'Uno local apply gate');
mustContain(unoService, 'PeerTimeoutMs', 'Uno calibration fail-closed owner');
mustContain(unoService, 'motorPermit', 'Uno calibration motor permit owner');

for (const [name, text] of [
  ['collector header', collectorH],
  ['collector cpp', collectorCpp],
  ['raw protocol header', rawProtocolH],
  ['raw protocol cpp', rawProtocolCpp],
  ['done protocol header', doneProtocolH],
  ['done protocol cpp', doneProtocolCpp],
  ['completion adapter header', completionAdapterH],
  ['completion adapter cpp', completionAdapterCpp],
  ['Uno raw protocol header', unoProtocolH],
  ['Uno raw protocol cpp', unoProtocolCpp],
  ['Uno compact completion formatter', unoDoneFormatter],
  ['Uno raw bridge header', unoBridgeH],
  ['Uno raw bridge cpp', unoBridgeCpp]
]) {
  mustNotContain(text, 'StartOrResume', `${name}`);
  mustNotContain(text, 'digitalWrite', `${name}`);
}

mustContain(handoff, 'CMP1|CAL_SAMPLE|BASELINE|raw|sequence|elapsed_ms|C|CRC', 'raw migration handoff');
mustContain(handoff, 'normal winding realtime turn count remains Uno-local', 'raw migration handoff');

console.log('Hall raw migration ownership/wire contracts: OK');
