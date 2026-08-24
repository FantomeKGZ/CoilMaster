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
mustContain(analyzerCpp, 'analyzeSummary', 'ESP32 analyzer owner');

// Realtime/safety ownership must stay on Uno throughout migration.
mustContain(unoMain, 'processExternalStart', 'Uno safety runtime');
mustContain(unoMain, 'ssr.update', 'Uno SSR owner');
mustContain(unoMain, 'processTurnSource', 'Uno turn counter owner');
mustContain(unoMain, 'WaitingApplyConfirm', 'Uno local apply gate');
mustContain(unoService, 'PeerTimeoutMs', 'Uno calibration fail-closed owner');
mustContain(unoService, 'motorPermit', 'Uno calibration motor permit owner');

// ESP32 collector must not gain actuator APIs.
mustNotContain(collectorH, 'SSR', 'ESP32 raw collector');
mustNotContain(collectorH, 'StartOrResume', 'ESP32 raw collector');
mustNotContain(collectorCpp, 'digitalWrite', 'ESP32 raw collector');

mustContain(handoff, 'CMP1|CAL_SAMPLE|BASELINE|raw|sequence|elapsed_ms|C|CRC', 'raw migration handoff');
mustContain(handoff, 'normal winding realtime turn count remains Uno-local', 'raw migration handoff');
mustContain(handoff, '32725501435', 'raw migration verified baseline');

console.log('Hall raw migration ownership contracts: OK');
