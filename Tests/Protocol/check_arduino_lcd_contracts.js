const fs = require('fs');
const path = require('path');

const root = path.resolve(__dirname, '../..');
const sourcePath = 'Arduino/CM_Lcd1602View.cpp';
const source = fs.readFileSync(path.join(root, sourcePath), 'utf8');
const entrypointPath = 'firmware/arduino/src/main.cpp';
const entrypoint = fs.readFileSync(path.join(root, entrypointPath), 'utf8');
const failures = [];

function requireText(text, description) {
  if (!source.includes(text)) failures.push(description + ': ' + text);
}

// Row 1 has a permanent three-character synchronization marker. Its ownership
// must stay explicit so operator text is designed for the remaining 13 columns.
for (const [text, description] of [
  ['constexpr uint8_t SyncMarkerStart = 13U;', 'sync marker start moved or disappeared'],
  ['static_assert(SyncMarkerStart + 3U == DisplayColumns', 'three-column marker ownership is not enforced'],
  ['line[SyncMarkerStart] = \' \';', 'marker separator is not written through the reserved column constant'],
  ['line[SyncMarkerStart + 1U]', 'marker status column is not based on the reserved range'],
  ['line[SyncMarkerStart + 2U]', 'marker count/status column is not based on the reserved range']
]) {
  requireText(text, description);
}

// These labels are deliberately compact enough for columns 0..12. The previous
// full-width labels were visibly overwritten by OK/Px/Ex in columns 13..15.
for (const label of [
  'KOL-VO KAT.?',
  'VITKI ',
  'GOTOV ',
  'NAMOTKA ',
  'PAUZA ',
  'RUCHNOY REZH.',
  'KAT. GOTOVA',
  'GOTOVO ',
  'OSHIBKA'
]) {
  requireText('F("' + label + '")', 'compact row-1 label missing');
}

for (const forbidden of [
  'KOL-VO KATUSHEK?',
  'RUCHNOY REZHIM',
  'KATUSHKA GOTOVA',
  'OSHIBKA SISTEMY'
]) {
  if (source.includes(forbidden)) {
    failures.push('full-width row-1 label can collide with sync marker: ' + forbidden);
  }
}

// The real LCD was detected at 0x27 and passed an isolated library test. Keep
// I2C/LCD initialization ahead of UART/EEPROM/services so the operator sees a
// bounded boot stage instead of an uninitialized row of blocks.
for (const [text, description] of [
  ['#include <Wire.h>', 'explicit Wire dependency missing'],
  ['Wire.begin();', 'I2C bus is not explicitly initialized'],
  ['lcdView.begin();', 'LCD view is not initialized'],
  ['showLcdBootStage(F("LCD"))', 'first visible LCD boot stage missing'],
  ['showLcdBootStage(F("UART"))', 'UART boot stage missing'],
  ['showLcdBootStage(F("EEPROM"))', 'EEPROM boot stage missing'],
  ['showLcdBootStage(F("SETTINGS"))', 'settings boot stage missing'],
  ['showLcdBootStage(F("STATE"))', 'state boot stage missing']
]) {
  if (!entrypoint.includes(text)) failures.push(description + ': ' + text);
}
if (entrypoint.indexOf('Wire.begin();') > entrypoint.indexOf('espTransport.begin();')) {
  failures.push('LCD/I2C must initialize before UART startup');
}
if (entrypoint.indexOf('lcdView.begin();') > entrypoint.indexOf('restorePersistentState();')) {
  failures.push('LCD must initialize before EEPROM restore');
}
if ((entrypoint.match(/lcdView\.begin\(\);/g) || []).length !== 1) {
  failures.push('LCD view must be initialized exactly once');
}

// Production diagnostics are disabled, so reset provenance must remain visible
// on the LCD and every checkpoint must be readable during a reset loop. The SSR
// safety owner is initialized exactly once before those bounded holds.
for (const [text, description] of [
  ['cmResetFlags = static_cast<uint8_t>(MCUSR & 0x1FU);', 'defined AVR reset flags are not captured and masked in production'],
  ['CM BOOT RST:', 'LCD reset-code heading missing'],
  ['delay(350U);', 'boot checkpoint is not held long enough to read'],
  ['showLcdBootStage(F("SSR SAFE OFF"))', 'fail-safe SSR checkpoint missing']
]) {
  if (!entrypoint.includes(text)) failures.push(description + ': ' + text);
}
if ((entrypoint.match(/ssr\.begin\(\);/g) || []).length !== 1) {
  failures.push('SSR must be initialized exactly once');
}
if (entrypoint.indexOf('ssr.begin();') > entrypoint.indexOf('Serial.begin(115200);')) {
  failures.push('SSR must be forced fail-safe before startup diagnostics');
}
if (entrypoint.indexOf('ssr.begin();') > entrypoint.indexOf('Wire.begin();')) {
  failures.push('SSR must be fail-safe before LCD checkpoint delays');
}

// Reset-loop localization must survive the reset and identify the last loop
// subsystem without writing EEPROM on every pass.
for (const [text, description] of [
  ['MCUSR & 0x1FU', 'reserved reset bits are not masked'],
  ['section(".noinit")', 'reset-loop evidence is not retained'],
  ['showPreviousLoopPhase();', 'previous loop phase is not shown after LCD init'],
  ['PREV LOOP:', 'previous-loop LCD heading missing'],
  ['cmLastLoopPhase = 6U;', 'UART loop checkpoint missing'],
  ['cmLastLoopPhase = 11U;', 'output loop checkpoint missing'],
  ['cmLastLoopPhase = 13U;', 'completed-loop checkpoint missing']
]) {
  if (!entrypoint.includes(text)) failures.push(description + ': ' + text);
}

const features = fs.readFileSync(path.join(root, 'Arduino/Config/CM_Features.h'), 'utf8');
const platformio = fs.readFileSync(path.join(root, 'platformio.ini'), 'utf8');
for (const [present, description] of [
  [features.includes('#define CM_FEATURE_BOOT_DIAGNOSTICS 0'), 'bounded boot diagnostic default missing'],
  [!platformio.includes('[env:uno_diagnostic]'), 'retired Uno diagnostic environment must stay removed'],
  [!platformio.includes('-DCM_FEATURE_BOOT_DIAGNOSTICS=1'), 'retired Uno diagnostic build flag must stay removed'],
  [!platformio.includes('-DCM_FEATURE_DIAGNOSTICS=1'), 'full verbose diagnostics must not be enabled for production Uno'],
  [entrypoint.includes('HardwareSerial& cmBootSerial = Serial;'), 'USB boot serial owner missing']
]) {
  if (!present) failures.push(description);
}

if (failures.length) {
  console.error(failures.map(item => sourcePath + ': ' + item).join('\n'));
  process.exit(1);
}

console.log('Arduino LCD contracts OK: layout/startup/reset provenance stay bounded and the retired Uno diagnostic profile stays removed.');
