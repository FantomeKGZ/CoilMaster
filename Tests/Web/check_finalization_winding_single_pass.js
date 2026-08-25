const fs = require('fs');
const path = require('path');

const root = path.resolve(__dirname, '../..');
const relative = 'firmware/esp32/src/CM_RepairFinalizationGuard.cpp';
const source = fs.readFileSync(path.join(root, relative), 'utf8');
const failures = [];

for (const required of [
  '#include "CM_WindingJournalTransitionAudit.h"',
  'WindingJournalTransitionAudit::validate(storage)',
  'RepairFinalizationCheck::WindingStorageUnavailable',
  'RepairFinalizationCheck::WindingIntegrityFailed',
  'WireWriteOffCoverageAudit::check(storage, repairId)'
]) {
  if (!source.includes(required)) failures.push(relative + ': required finalization invariant missing: ' + required);
}

for (const forbidden of [
  '#include "CM_WindingJournalQuery.h"',
  'WindingJournalQuery history(storage)',
  'history.validateAll()',
  'query.validateAll()'
]) {
  if (source.includes(forbidden)) failures.push(relative + ': redundant winding journal pre-scan returned: ' + forbidden);
}

if (failures.length) {
  console.error(failures.join('\n'));
  process.exit(1);
}

console.log('Finalization winding contracts OK: authoritative schema/transition validation remains single-pass before exact-run write-off coverage.');
