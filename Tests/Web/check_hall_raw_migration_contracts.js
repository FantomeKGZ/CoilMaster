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
const unoProtocolH = read('Arduino/CM_HallCalibrationProtocol.h');
const unoProtocolCpp = read('Arduino/CM_HallCalibrationProtocol.cpp');
const analyzerCpp = read('firmware/esp32/src/CM_HallCalibrationAnalyzer.cpp');
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

mustContain(unoProtocolH, 'HallCalibrationSamplePhase', 'Uno raw protocol');
mustContain(unoProtocolH, 'formatSample', 'Uno raw protocol');
mustContain(unoProtocolCpp, 'CMP1|CAL_SAMPLE|%S|%u|%u|%lu|C', 'Uno raw protocol');
mustContain(unoProtocolCpp, 'PSTR("BASELINE")', 'Uno raw protocol');
mustContain(unoProtocolCpp, 'PSTR("RUN")', 'Uno raw protocol');
mustContain(unoProtocolCpp, 'rawAdc > 1023U', 'Uno raw protocol');
mustContain(unoProtocolCpp, 'appendCrc(output, outputSize, length)', 'Uno raw protocol');

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
  ['Uno raw protocol header', unoProtocolH],
  ['Uno raw protocol cpp', unoProtocolCpp]
]) {
  mustNotContain(text, 'StartOrResume', `${name}`);
  mustNotContain(text, 'digitalWrite', `${name}`);
}

mustContain(handoff, 'CMP1|CAL_SAMPLE|BASELINE|raw|sequence|elapsed_ms|C|CRC', 'raw migration handoff');
mustContain(handoff, 'normal winding realtime turn count remains Uno-local', 'raw migration handoff');
mustContain(handoff, '32725501435', 'raw migration verified baseline');

console.log('Hall raw migration ownership/wire contracts: OK');
