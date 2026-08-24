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
const receiverH = read('firmware/esp32/src/CM_UartEventReceiver.h');
const receiverCpp = read('firmware/esp32/src/CM_UartEventReceiver.cpp');
const unoProtocolH = read('Arduino/CM_HallCalibrationProtocol.h');
const unoProtocolCpp = read('Arduino/CM_HallCalibrationProtocol.cpp');
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

mustContain(receiverH, 'HallCalibrationRawCollector m_hallCalibrationRaw', 'ESP32 raw receiver');
mustContain(receiverCpp, 'CMP1|CAL_SAMPLE|', 'ESP32 raw receiver');
mustContain(receiverCpp, 'HallCalibrationRawProtocol::parseSample', 'ESP32 raw receiver');
mustContain(receiverCpp, 'm_hallCalibrationRaw.addBaselineSample', 'ESP32 raw receiver');
mustContain(receiverCpp, 'm_hallCalibrationRaw.addRunSample', 'ESP32 raw receiver');
mustContain(receiverCpp, 'result.baselineAdc = rawSummary.baselineAdc', 'ESP32 raw result ownership');
mustContain(receiverCpp, 'result.minAdc = rawSummary.minAdc', 'ESP32 raw result ownership');
mustContain(receiverCpp, 'result.maxAdc = rawSummary.maxAdc', 'ESP32 raw result ownership');

mustContain(unoProtocolH, 'HallCalibrationSamplePhase', 'Uno raw protocol');
mustContain(unoProtocolH, 'formatSample', 'Uno raw protocol');
mustContain(unoProtocolCpp, 'CMP1|CAL_SAMPLE|%S|%u|%u|%lu|C', 'Uno raw protocol');
mustContain(unoProtocolCpp, 'PSTR("BASELINE")', 'Uno raw protocol');
mustContain(unoProtocolCpp, 'PSTR("RUN")', 'Uno raw protocol');
mustContain(unoProtocolCpp, 'rawAdc > 1023U', 'Uno raw protocol');
mustContain(unoProtocolCpp, 'appendCrc(output, outputSize, length)', 'Uno raw protocol');

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

// Uno result storage is identity-only; legacy wire statistics are literal zeroes.
for (const field of ['baselineAdc', 'minAdc', 'maxAdc', 'sampleCount', 'durationMs']) {
  mustNotContain(unoServiceH, field, 'Uno identity-only result');
}
mustContain(
  unoProtocolCpp,
  'CMP1|CAL_RESULT|INVALID|0|0|0|0|0|RISING|0|0|%lu|C',
  'Uno identity-only legacy result frame'
);
mustNotContain(unoProtocolCpp, 'result.baselineAdc', 'Uno legacy result formatter');
mustNotContain(unoProtocolCpp, 'result.minAdc', 'Uno legacy result formatter');
mustNotContain(unoProtocolCpp, 'result.maxAdc', 'Uno legacy result formatter');
mustNotContain(unoProtocolCpp, 'result.sampleCount', 'Uno legacy result formatter');
mustNotContain(unoProtocolCpp, 'result.durationMs', 'Uno legacy result formatter');

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
  ['Uno raw protocol header', unoProtocolH],
  ['Uno raw protocol cpp', unoProtocolCpp],
  ['Uno raw bridge header', unoBridgeH],
  ['Uno raw bridge cpp', unoBridgeCpp]
]) {
  mustNotContain(text, 'StartOrResume', `${name}`);
  mustNotContain(text, 'digitalWrite', `${name}`);
}

mustContain(handoff, 'CMP1|CAL_SAMPLE|BASELINE|raw|sequence|elapsed_ms|C|CRC', 'raw migration handoff');
mustContain(handoff, 'normal winding realtime turn count remains Uno-local', 'raw migration handoff');

console.log('Hall raw migration ownership/wire contracts: OK');
