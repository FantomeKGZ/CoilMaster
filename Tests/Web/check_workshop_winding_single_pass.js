const fs = require('fs');
const path = require('path');

const root = path.resolve(__dirname, '..', '..');
const materialPath = path.join(root, 'firmware', 'esp32', 'src', 'CM_MaterialPersistenceIntegrityAudit.cpp');
const source = fs.readFileSync(materialPath, 'utf8');

function requireText(text, description) {
  if (!source.includes(text)) throw new Error(`Missing ${description}: ${text}`);
}

for (const [text, description] of [
  ['#include "CM_WindingPersistenceIntegrityAudit.h"', 'single-pass winding persistence dependency'],
  ['WindingPersistenceIntegrityAudit::check(storage)', 'single-pass winding persistence call'],
  ['WarehousePersistenceIntegrityAudit::check(storage)', 'standalone warehouse integrity'],
  ['PersistentIdIntegrityAudit::check(storage)', 'standalone allocator integrity'],
  ['WindingSessionPersistenceIntegrityAudit::check(storage)', 'standalone winding session integrity'],
  ['BackupBusinessDataIntegrityAudit::check(storage)', 'authoritative workshop registry and pricing integrity'],
  ['workshopDirectoryShapeValid(storage)', 'fail-closed workshop directory shape'],
  ['RunWireAccountingIntegrityAudit::check(storage)', 'standalone RUN_WIRE cross-log integrity']
]) {
  requireText(text, description);
}

for (const forbidden of [
  '#include "CM_WindingJournalQuery.h"',
  '#include "CM_WindingJournalTransitionAudit.h"',
  'WindingJournalQuery query(',
  'query.validateAll(',
  'WindingJournalTransitionAudit::validate(storage)',
  'WorkshopPersistenceIntegrityAudit',
  'RepairPricingIntegrityAudit'
]) {
  if (source.includes(forbidden)) {
    throw new Error(`Standalone material audit must not restore retired/pre-pass dependency: ${forbidden}`);
  }
}

console.log('Workshop winding single-pass contracts OK: broad standalone integrity remains fail-closed through direct authoritative owners while winding schema/transition validation is delegated to the one-pass audit.');
