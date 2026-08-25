const fs = require('fs');
const path = require('path');

const root = path.resolve(__dirname, '..', '..');
const workshopPath = path.join(root, 'firmware', 'esp32', 'src', 'CM_WorkshopPersistenceIntegrityAudit.cpp');
const source = fs.readFileSync(workshopPath, 'utf8');

function requireText(text, description) {
  if (!source.includes(text)) throw new Error(`Missing ${description}: ${text}`);
}

for (const [text, description] of [
  ['#include "CM_WindingPersistenceIntegrityAudit.h"', 'single-pass winding persistence dependency'],
  ['WindingPersistenceIntegrityAudit::check(storage)', 'single-pass winding persistence call'],
  ['WarehousePersistenceIntegrityAudit::check(storage)', 'standalone warehouse integrity'],
  ['PersistentIdIntegrityAudit::check(storage)', 'standalone allocator integrity'],
  ['WindingSessionPersistenceIntegrityAudit::check(storage)', 'standalone winding session integrity'],
  ['RepairRegistry registry(storage)', 'workshop registry integrity']
]) {
  requireText(text, description);
}

for (const forbidden of [
  '#include "CM_WindingJournalQuery.h"',
  '#include "CM_WindingJournalTransitionAudit.h"',
  'WindingJournalQuery query(',
  'query.validateAll(',
  'WindingJournalTransitionAudit::validate(storage)'
]) {
  if (source.includes(forbidden)) {
    throw new Error(`Workshop audit must not restore direct winding journal pre-pass: ${forbidden}`);
  }
}

console.log('Workshop winding single-pass contracts OK: broad standalone integrity remains fail-closed while winding schema/transition validation is delegated to the one-pass audit.');
