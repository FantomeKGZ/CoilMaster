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

function forbid(text, needle, label) {
  if (text.includes(needle)) throw new Error(`forbidden ${label}: ${needle}`);
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

const buildPendingStart = source.indexOf('bool MaterialRequestWarehouseCoordinator::buildPending(');
const applyLedgerStart = source.indexOf('bool MaterialRequestWarehouseCoordinator::applyLedger(', buildPendingStart);
if (buildPendingStart < 0 || applyLedgerStart <= buildPendingStart) {
  throw new Error('buildPending body missing');
}
const buildPending = source.slice(buildPendingStart, applyLedgerStart);
must(buildPending,
  'requestMatchesRepair(requestedMovement.materialRequestId,',
  'authoritative exact request/repair validation before status-only lookup');
must(buildPending,
  'knownRequestAllowsWarehouseMutation(requestedMovement.materialRequestId)',
  'known-request status-only warehouse lifecycle gate');
forbid(buildPending,
  'requestAllowsWarehouseMutation(requestedMovement.materialRequestId)',
  'fresh warehouse path full request-journal status resolver');

const knownStart = source.indexOf('bool MaterialRequestWarehouseCoordinator::knownRequestAllowsWarehouseMutation(');
const knownEnd = source.indexOf('bool MaterialRequestWarehouseCoordinator::isRemoveMutation(', knownStart);
if (knownStart < 0 || knownEnd <= knownStart) {
  throw new Error('known-request warehouse lifecycle helper missing');
}
const known = source.slice(knownStart, knownEnd);
must(known,
  'm_storage.open(MaterialRequestStatusStore::Path, FILE_READ)',
  'status-only journal scan for already-validated request');
must(known,
  'transitionId <= previousTransitionId',
  'global status transition ordering validation');
must(known,
  'MaterialRequestStatusStore::validTransition(fromStatus, toStatus)',
  'canonical lifecycle transition validation');
must(known,
  'fromStatus != state.status || state.transitionCount == 0xFFFFFFFFUL',
  'exact request lifecycle chain validation');
forbid(known, 'appendByIdJson(', 'duplicate request journal lookup');
forbid(known, 'm_statuses.resolve(', 'duplicate request existence scan through general resolver');
forbid(known, 'material-requests.ndjson', 'direct duplicate request journal scan');
forbid(known, 'std::vector', 'unbounded known-request state');
forbid(known, 'readString()', 'whole-file status buffering');

const recoverStart = source.indexOf('bool MaterialRequestWarehouseCoordinator::recover()');
const recoverEnd = source.indexOf('bool MaterialRequestWarehouseCoordinator::buildPending(', recoverStart);
if (recoverStart < 0 || recoverEnd <= recoverStart) {
  throw new Error('warehouse recovery body missing');
}
const recover = source.slice(recoverStart, recoverEnd);
must(recover,
  'requestAllowsWarehouseMutation(pending.materialRequestId)',
  'recovery keeps full request existence validation');

const fullGateStart = source.indexOf('bool MaterialRequestWarehouseCoordinator::requestAllowsWarehouseMutation(');
const knownGateStart = source.indexOf('bool MaterialRequestWarehouseCoordinator::knownRequestAllowsWarehouseMutation(', fullGateStart);
if (fullGateStart < 0 || knownGateStart <= fullGateStart) {
  throw new Error('full warehouse lifecycle gate missing');
}
const fullGate = source.slice(fullGateStart, knownGateStart);
must(fullGate,
  'm_statuses.resolve(materialRequestId, state, found)',
  'recovery/general gate retains authoritative request existence lookup');

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

require('./check_spool_material_bridge_store.js');
require('./check_spool_material_bridge_web.js');
require('./check_run_wire_issue_transaction.js');

console.log('Material Request warehouse coordinator/runtime API contracts OK; fresh exact-request path scans status only while recovery keeps full request existence validation');
