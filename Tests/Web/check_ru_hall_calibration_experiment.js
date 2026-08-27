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

const serviceHeader = read('Arduino/CM_HallCalibrationService.h');
const serviceCpp = read('Arduino/CM_HallCalibrationService.cpp');
const arduinoMain = read('firmware/arduino/src/main.cpp');
const sharedWeb = read('firmware/esp32/web/shared/settings-hall-calibration.js');
const lcdView = read('Arduino/CM_Lcd1602View.cpp');

mustContain(serviceHeader, 'RunDurationMs = 15000UL', 'RU Hall duration');
mustContain(serviceHeader, 'AbsoluteRunTimeoutMs = 17000UL', 'RU Hall timeout guard');
mustContain(serviceHeader, 'PeerTimeoutMs = 3000UL', 'RU Hall peer fail-closed guard');
mustContain(serviceHeader, 'SampleIntervalMs = 10U', 'RU Hall fast ADC sampling');
mustContain(serviceHeader, 'RunPublishIntervalMs = 250U', 'RU Hall UART publish throttle');
mustContain(serviceHeader, 'BaselineSampleIntervalMs = 100U', 'RU Hall baseline UART throttle');

mustContain(serviceCpp, 'm_state = HallCalibrationState::WaitingLocalConfirm;', 'ARM compatibility state');
mustContain(serviceCpp, 's_displayState = HallCalibrationState::ArmedWaitingPhysicalStart;', 'RU LCD immediate ready state');
mustContain(serviceCpp, 'if (m_state == HallCalibrationState::WaitingLocalConfirm)', 'automatic local-confirm bridge');
mustContain(serviceCpp, '(void)confirmLocal(nowMs);', 'automatic local-confirm bridge');
mustContain(serviceCpp, '!baselineReady()', 'physical start baseline interlock');
mustContain(serviceCpp, 'return m_state == HallCalibrationState::Running;', 'motor permit only while running');
mustContain(serviceCpp, 'if (raw < m_runWindowMin) m_runWindowMin = raw;', 'RU Hall window minimum capture');
mustContain(serviceCpp, 'if (raw > m_runWindowMax) m_runWindowMax = raw;', 'RU Hall window maximum capture');
mustContain(serviceCpp, 'publishRunWindow(nowMs);', 'RU Hall decimated UART publish');
mustContain(serviceCpp, 'm_runWindowMax != m_runWindowMin', 'RU Hall avoid duplicate extrema frame');

mustContain(arduinoMain, 'startHallCalibrationFromLocalControl', 'shared local Hall start helper');
mustContain(arduinoMain, "if (key == 'A')", 'keypad A Hall start');
mustContain(arduinoMain, 'startHallCalibrationFromLocalControl(millis())', 'keypad shared Hall start helper');
mustContain(arduinoMain, 'startHallCalibrationFromLocalControl(nowMs)', 'physical START shared Hall start helper');
mustContain(arduinoMain, 'hallCalibration.motorPermit()', 'SSR Hall permit interlock');
mustContain(arduinoMain, 'ssr.forceOff();', 'Hall fail-safe SSR off path');

mustContain(lcdView, 'HALL TEST READY', 'RU Hall ready LCD');
mustContain(lcdView, 'A OR START', 'RU Hall local start LCD');
mustContain(lcdView, 'HALL TEST RUN', 'RU Hall running LCD');
mustContain(lcdView, 'LEFT ', 'RU Hall countdown LCD');
mustContain(lcdView, 'SAVE HALL CFG?', 'RU Hall apply LCD');

mustContain(sharedWeb, 'A на клавиатуре или отдельной физической START', 'Hall web local start instruction');
mustContain(sharedWeb, 'Калибровка выполняется 15 секунд', 'Hall web duration instruction');
mustContain(sharedWeb, 'Дополнительное подтверждение # не требуется', 'Hall web no-start-confirm instruction');
mustNotContain(sharedWeb, 'Подтвердите калибровку клавишей # на Arduino', 'obsolete Hall local-confirm instruction');
mustNotContain(sharedWeb, "CAL_URL+'/start'", 'ESP32 must not expose Hall motor start');
mustNotContain(sharedWeb, '/ssr', 'ESP32 web must not control SSR');

console.log('RU Hall calibration experiment contracts: OK');
