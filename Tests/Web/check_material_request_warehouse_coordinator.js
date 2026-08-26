const fs = require('fs');

const header = fs.readFileSync('firmware/esp32/src/CM_MaterialRequestWarehouseCoordinator.h', 'utf8');
const source = fs.readFileSync('firmware/esp32/src/CM_MaterialRequestWarehouseCoordinator.cpp', 'utf8');
const webHeader = fs.readFileSync('firmware/esp32/src/CM_MaterialRequestWeb.h', 'utf8');
const webSource = fs.readFileSync('firmware/esp32/src/CM_MaterialRequestWeb.cpp', 'utf8');
const runtimeSource = fs.readFileSync('firmware/esp32/src/CM_MaterialRequestRuntime.cpp', 'utf8');
const repairHeader = fs.readFileSync('firmware/esp32/src/CM_RepairRegistry.h', 'utf8');
const repairLookup = fs.readFileSync('firmware/esp32/src/CM_RepairRegistryLookup.cpp', 'utf8');
const repairWeb = fs.readFileSync('firmware/esp32/src/CM_RepairRegistryWeb.cpp', 'utf8');

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

must(webHeader, 'class MaterialRequestWeb', 'bounded Material Request web class');
must(webSource, '"/api/material-requests"', 'request create/list route');
must(webSource, '"/api/material-requests/item"', 'request item route');
must(webSource, '"/api/material-requests/movements"', 'movement history route');
must(webSource, '"/api/material-requests/status"', 'status routes');
must(webSource, '"/api/material-requests/warehouse"', 'warehouse mutation route');
must(webSource, 'explicit_confirmation_required', 'explicit operator confirmation');
must(webSource, 'm_warehouse.execute(', 'warehouse mutation only through coordinator');
must(webSource, 'm_repairs.loadRepairIdentity(', 'server-side repair identity lookup');
must(repairHeader, 'struct RepairIdentity', 'repair identity contract');
must(repairLookup, 'RepairRegistry::loadRepairIdentity', 'repair identity implementation');
must(runtimeSource, 'warehouse.begin()', 'coordinator recovery before route registration');
must(runtimeSource, 'web.begin();', 'route registration after successful runtime init');
must(repairWeb, 'beginMaterialRequestRuntime(m_server, m_registry);', 'production runtime bootstrap');

if (source.includes('RUN_COMPLETED')) {
  throw new Error('warehouse coordinator must not couple mutation to RUN_COMPLETED');
}
if (source.includes('m_statuses.transition')) {
  throw new Error('warehouse coordinator must not rewrite Material Request lifecycle implicitly');
}
if (webSource.includes('.confirmUsage(') || webSource.includes('.adjustMaterial(')) {
  throw new Error('Material Request Web must never mutate MaterialLedger directly');
}
if (webSource.includes('price_per_unit_minor') || webSource.includes('unit_cost_minor"')) {
  throw new Error('Material Request warehouse Web must not accept operator-supplied material pricing');
}

// Keep checkpoint 121 RUN_WIRE atomicity in the existing mandatory CMP step.
require('./check_run_wire_issue_transaction.js');

console.log('Material Request warehouse coordinator/runtime API contracts OK');
