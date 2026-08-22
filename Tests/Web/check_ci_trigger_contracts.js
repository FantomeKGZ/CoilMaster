const fs = require('fs');

const arduino = fs.readFileSync('.github/workflows/arduino-uno-build.yml', 'utf8');
const esp32 = fs.readFileSync('.github/workflows/esp32-build.yml', 'utf8');
const protocol = fs.readFileSync('.github/workflows/cmp-protocol-tests.yml', 'utf8');
const transport = fs.readFileSync('Arduino/CM_UartEventTransport.cpp', 'utf8');

function requireText(source, needle, message) {
  if (!source.includes(needle)) throw new Error(message);
}

requireText(transport, '../Shared/CMP1Text/CM_Cmp1Crc.h',
  'Arduino transport no longer exposes the shared CRC dependency expected by this audit');

for (const [name, source] of [
  ['Arduino Uno Build', arduino],
  ['ESP32 Build', esp32],
  ['CMP Protocol Tests', protocol],
]) {
  requireText(source, '- "Shared/**"',
    `${name} must run when shared protocol/CRC code changes`);
  requireText(source, 'cmp-protocol-v1',
    `${name} must include the cmp-protocol-v1 source-of-truth branch`);
}

requireText(arduino, '- "Arduino/**"',
  'Arduino build must run for Arduino transport changes');
requireText(arduino, '- "Core/**"',
  'Arduino build must run for Core state-machine changes');
requireText(arduino, '- "firmware/arduino/**"',
  'Arduino build must run for production UNO entrypoint changes');
requireText(arduino, 'pio run -e uno',
  'Arduino workflow must compile the production UNO PlatformIO environment');

requireText(esp32, '- "firmware/esp32/**"',
  'ESP32 build must run for ESP32 firmware/web changes');
requireText(esp32, '- "scripts/platformio_build_id.py"',
  'ESP32 build must run when build-identity generation changes');
requireText(esp32, 'pio run -e esp32',
  'ESP32 workflow must compile the production ESP32 PlatformIO environment');

for (const path of [
  '- "Tests/Protocol/**"',
  '- "Tests/Web/**"',
  '- "Arduino/**"',
  '- "Core/**"',
  '- "firmware/arduino/**"',
  '- "firmware/esp32/src/**"',
  '- "firmware/esp32/web/**"',
]) {
  requireText(protocol, path,
    `CMP Protocol Tests pull-request trigger missing ${path}`);
}

console.log('CI trigger contracts OK: shared code and production sources reach their required build/test gates.');
