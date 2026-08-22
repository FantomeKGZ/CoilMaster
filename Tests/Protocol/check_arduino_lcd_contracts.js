const fs = require('fs');
const path = require('path');

const root = path.resolve(__dirname, '../..');
const sourcePath = 'Arduino/CM_Lcd1602View.cpp';
const source = fs.readFileSync(path.join(root, sourcePath), 'utf8');
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

if (failures.length) {
  console.error(failures.map(item => sourcePath + ': ' + item).join('\n'));
  process.exit(1);
}

console.log('Arduino LCD contracts OK: row-1 operator text reserves columns 13..15 for the synchronization marker.');
