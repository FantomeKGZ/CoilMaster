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
const eepromSource = fs.readFileSync('Arduino/CM_EepromPersistence.cpp', 'utf8');

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
  '#include <Keypad.h>',
  'Keypad keypad = Keypad(makeKeymap(KeyMap), RowPins, ColPins, 4, 4);',
  'const char key = keypad.getKey();',
  'if (key == NO_KEY) return;',
]) {
  if (!productionMain.includes(required)) {
    failures.push(`${productionMainPath}: proven Keypad runtime missing: ${required}`);
  }
}
if (!platformio.includes('chris--a/Keypad @ ^3.1.1')) {
  failures.push(`${platformioPath}: proven Keypad dependency missing`);
}
for (const forbidden of [
  'void beginKeypad()',
  'char scanKeypadRaw()',
  'char pollKeypad()',
  'KeypadDebounceMs = 25U',
]) {
  if (productionMain.includes(forbidden)) {
    failures.push(`${productionMainPath}: compact keypad scanner must remain removed: ${forbidden}`);
  }
}

for (const required of [
  'if (!metadataValidInEeprom())',
  'uint16_t EepromPersistence::metadataCrcInEeprom() const',
  'bool EepromPersistence::metadataValidInEeprom() const',
  'void EepromPersistence::resetMetadataInEeprom() const'
]) {
  if (!eepromSource.includes(required)) {
    failures.push(`Arduino/CM_EepromPersistence.cpp: streaming boot metadata contract missing: ${required}`);
  }
}
const beginBody = eepromSource.slice(
  eepromSource.indexOf('void EepromPersistence::begin()'),
  eepromSource.indexOf('uint32_t EepromPersistence::nextSessionId()')
);
if (beginBody.includes('StoredMetadataState metadata;')) {
  failures.push('Arduino/CM_EepromPersistence.cpp: startup must not place the full metadata sidecar on the Uno stack');
}

if (failures.length) {
  console.error(failures.join('\n'));
  process.exit(1);
}

console.log('Arduino cleanup contract OK: obsolete artifacts stay removed, proven Keypad runtime is restored, and PlatformIO production main remains authoritative.');
