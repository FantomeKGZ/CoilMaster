const fs = require('fs');

const obsoleteArtifacts = [
  'Arduino/CoilMaster_Arduino.ino',
  'Arduino/Config/CM_Version.h',
];
const platformioPath = 'platformio.ini';
const productionMainPath = 'firmware/arduino/src/main.cpp';

const failures = [];

for (const obsoletePath of obsoleteArtifacts) {
  if (fs.existsSync(obsoletePath)) {
    failures.push(`${obsoletePath}: obsolete Arduino artifact must remain removed`);
  }
}

const platformio = fs.readFileSync(platformioPath, 'utf8');
const productionMain = fs.readFileSync(productionMainPath, 'utf8');

for (const required of [
  '+<Core/*.cpp>',
  '+<Arduino/*.cpp>',
  '+<firmware/arduino/src/main.cpp>',
]) {
  if (!platformio.includes(required)) {
    failures.push(`${platformioPath}: missing authoritative Arduino build owner: ${required}`);
  }
}

for (const required of [
  'CM_UartEventTransport.h',
  'CM_EepromPersistence.h',
  'CM_HardwareSettingsController.h',
  'processExternalStart',
  'processRemoteJobs',
  'processCoreEvents',
]) {
  if (!productionMain.includes(required)) {
    failures.push(`${productionMainPath}: production entrypoint contract missing: ${required}`);
  }
}


for (const required of [
  'void beginKeypad()',
  'char scanKeypadRaw()',
  'char pollKeypad()',
  'pgm_read_byte(&KeyMap[index])',
  'KeypadDebounceMs = 25U'
]) {
  if (!productionMain.includes(required)) {
    failures.push(`${productionMainPath}: compact keypad scanner missing: ${required}`);
  }
}
for (const forbidden of ['#include <Keypad.h>', 'Keypad keypad', 'keypad.getKey()', 'chris--a/Keypad']) {
  if (productionMain.includes(forbidden) || platformio.includes(forbidden)) {
    failures.push(`obsolete heap-heavy Keypad owner remains: ${forbidden}`);
  }
}

if (failures.length) {
  console.error(failures.join('\n'));
  process.exit(1);
}

console.log('Arduino cleanup contract OK: obsolete artifacts stay removed and PlatformIO production main remains authoritative.');
