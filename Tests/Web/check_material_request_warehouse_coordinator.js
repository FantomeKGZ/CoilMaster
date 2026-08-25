const fs = require('fs');

const header = fs.readFileSync('firmware/esp32/src/CM_MaterialRequestWarehouseCoordinator.h', 'utf8');
const source = fs.readFileSync('firmware/esp32/src/CM_MaterialRequestWarehouseCoordinator.cpp', 'utf8');

function must(text, needle, label) {
  if (!text.includes(needle)) throw new Error(`missing ${label}: ${needle}`);
}

must(header, 'bool execute(', 'explicit execution API');
must(header, 'bool recover();', 'recovery API');
must(source, 'm_pending.save(pending)', 'durable intent before mutation');
must(source, 'm_movements.append(movement, movementId)', 'immutable movement evidence');
must(source, 'if (!applyLedger(pending, remaining)) return false;', 'physical ledger mutation after movement');

const movementPos = source.indexOf('m_movements.append(movement, movementId)');
const ledgerPos = source.indexOf('applyLedger(pending, remaining)');
if (movementPos < 0 || ledgerPos < 0 || movementPos >= ledgerPos) {
  throw new Error('warehouse transaction must persist movement before physical ledger mutation');
}

must(source, 'if (!movementFound && !ledgerFound)', 'neither-side recovery state');
must(source, 'if (!movementFound && ledgerFound) return false;', 'ledger-only fail-closed state');
must(source, 'if (movementFound && !ledgerFound)', 'movement-only recovery state');
must(source, 'return m_pending.clear();', 'pending clear after recovery');
must(source, 'state.status == "DRAFT" || state.status == "ISSUED"', 'warehouse lifecycle gate');
must(source, 'RepairLifecycle::isOpen', 'open repair gate');
must(source, 'MaterialRequestUnitAdapter::convert', 'server-side canonical unit/cost conversion');
must(source, 'm_ledger.loadActiveMaterialState', 'authoritative catalog state');
must(source, 'MRW_TX=', 'shared ledger transaction evidence');
must(source, 'esp_random()', 'transaction ref entropy');

if (source.includes('RUN_COMPLETED')) {
  throw new Error('warehouse coordinator must not couple mutation to RUN_COMPLETED');
}
if (source.includes('m_statuses.transition')) {
  throw new Error('warehouse coordinator must not rewrite Material Request lifecycle implicitly');
}

console.log('Material Request warehouse coordinator contracts OK');
