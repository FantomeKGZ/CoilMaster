const fs = require('fs');

const obsoleteSketch = 'Arduino/CoilMaster_Arduino.ino';
const platformioPath = 'platformio.ini';
const productionMainPath = 'firmware/arduino/src/main.cpp';

const failures = [];

if (fs.existsSync(obsoleteSketch)) {
  failures.push(`${obsoleteSketch}: obsolete parallel Arduino IDE entrypoint must remain removed`);
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

if (failures.length) {
  console.error(failures.join('\n'));
  process.exit(1);
}

console.log('Arduino entrypoint cleanup contract OK: obsolete .ino stays removed and PlatformIO production main remains authoritative.');
