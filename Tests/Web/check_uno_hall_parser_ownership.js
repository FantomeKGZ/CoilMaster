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

const hardwareHeader = read('Arduino/CM_HardwareControlProtocol.h');
const hardwareCpp = read('Arduino/CM_HardwareControlProtocol.cpp');
const hallHeader = read('Arduino/CM_HallCalibrationProtocol.h');
const hallCpp = read('Arduino/CM_HallCalibrationProtocol.cpp');
const uartCpp = read('Arduino/CM_UartEventTransport.cpp');

for (const type of [
  'ArmHallCalibration',
  'AbortHallCalibration',
  'GetHallCalibration',
  'StageHallCalibrationProposal'
]) {
  mustContain(hardwareHeader, type, 'shared hardware request types');
}

const worstHardwareWire = 'CMP1|HALL_STATE|65535|65535|65535|65535|65535|65535|65535|FALLING|1|RELEASE_DEBOUNCE|65535|4294967295|C|FFFF\n';
if (Buffer.byteLength(worstHardwareWire, 'ascii') !== 109) {
  throw new Error('Uno hardware TX bound fixture must remain 109 wire bytes');
}
mustContain(hardwareHeader, 'static constexpr size_t MaxFrameLength = 110U;', 'Uno hardware TX type-safe bound');
mustContain(hardwareHeader, 'static_assert(MaxFrameLength == 110U', 'Uno hardware TX bound compile guard');
mustContain(uartCpp, 'char frame[HardwareControlProtocol::MaxFrameLength];', 'Uno hardware TX bounded stack buffer');

mustContain(hardwareCpp, 'strcmp_P(category, PSTR("CAL")) == 0', 'shared hardware parser');
mustContain(hardwareCpp, 'PSTR("ARM")', 'shared hardware parser');
mustContain(hardwareCpp, 'PSTR("ABORT")', 'shared hardware parser');
mustContain(hardwareCpp, 'PSTR("GET")', 'shared hardware parser');
mustContain(hardwareCpp, 'PSTR("CAL_PROPOSAL")', 'shared hardware parser');

mustContain(hallHeader, 'Thin compatibility adapters only', 'Hall adapter contract');
mustContain(hallCpp, 'HardwareControlProtocol::parseRequest', 'Hall adapter implementation');
mustContain(hallCpp, 'HardwareControlRequestType::ArmHallCalibration', 'Hall command adapter');
mustContain(hallCpp, 'HardwareControlRequestType::AbortHallCalibration', 'Hall command adapter');
mustContain(hallCpp, 'HardwareControlRequestType::GetHallCalibration', 'Hall command adapter');
mustContain(hallCpp, 'HardwareControlRequestType::StageHallCalibrationProposal', 'Hall proposal adapter');

mustNotContain(hallCpp, 'verifyAndStripCrc(frame)', 'Hall parser ownership');
mustNotContain(hallCpp, 'strtok_r(frame', 'Hall parser ownership');
mustNotContain(hallCpp, 'strcmp_P(action', 'Hall parser ownership');
mustNotContain(hallCpp, 'parseUint32(', 'Hall parser ownership');
mustNotContain(hallCpp, 'parseUint16(', 'Hall parser ownership');

mustContain(uartCpp, 'HallCalibrationProtocol::parseRequest', 'UART compatibility routing');
mustContain(uartCpp, 'HallCalibrationProtocol::parseProposal', 'UART compatibility routing');

console.log('Uno Hall parser ownership and hardware TX bound: OK');
