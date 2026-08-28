const fs = require('fs');
const path = require('path');

const repoRoot = path.resolve(__dirname, '..', '..');
const headerPath = path.join(repoRoot, 'firmware', 'esp32', 'src', 'CM_RepairCosting.h');
const validationPath = path.join(repoRoot, 'firmware', 'esp32', 'src', 'CM_RepairCostingValidation.cpp');
const costingPath = path.join(repoRoot, 'firmware', 'esp32', 'src', 'CM_RepairCosting.cpp');
const webPath = path.join(repoRoot, 'firmware', 'esp32', 'src', 'CM_RepairCostingWeb.cpp');
const header = fs.readFileSync(headerPath, 'utf8');
const validation = fs.readFileSync(validationPath, 'utf8');
const costing = fs.readFileSync(costingPath, 'utf8');
const web = fs.readFileSync(webPath, 'utf8');

function requireText(source, text, description) {
  if (!source.includes(text)) {
    throw new Error(`Missing ${description}: ${text}`);
  }
}

const saveStart = costing.indexOf('bool RepairCosting::savePricing(');
const ensureStart = costing.indexOf('bool RepairCosting::ensureDirectories()', saveStart);
if (saveStart < 0 || ensureStart < 0) {
  throw new Error('Unable to isolate RepairCosting::savePricing implementation');
}
const savePricing = costing.slice(saveStart, ensureStart);
const loadStart = costing.indexOf('bool RepairCosting::load(');
if (loadStart < 0 || loadStart >= saveStart) {
  throw new Error('Unable to isolate RepairCosting::load implementation');
}
const load = costing.slice(loadStart, saveStart);

if (savePricing.includes('repairExists(')) {
  throw new Error('savePricing must not perform a duplicate repair identity scan before load()');
}
if (header.includes('bool repairExists(uint32_t repairId) const;')) {
  throw new Error('ambiguous RepairCosting repairExists wrapper returned');
}
if (validation.includes('bool RepairCosting::repairExists(uint32_t repairId) const')) {
  throw new Error('ambiguous RepairCosting repairExists implementation returned');
}
if (load.includes('repairExists(repairId)')) {
  throw new Error('RepairCosting::load must not collapse repair read failure into not-found');
}

requireText(
  savePricing,
  'if (!RepairLifecycle::isOpen(m_storage, repairId, repairOpen) || !repairOpen)',
  'repair lifecycle OPEN gate');
requireText(
  savePricing,
  'if (!load(repairId, current) || currency != current.currency ||',
  'authoritative costing load before pricing append');
requireText(
  header,
  'bool repairExists(uint32_t repairId, bool& found) const;',
  'fail-closed RepairCosting repair lookup API');
requireText(
  header,
  'bool loadKnownRepair(uint32_t repairId, RepairCostSummary& summary) const;',
  'known-repair read-only costing API');
requireText(
  validation,
  'bool RepairCosting::repairExists(uint32_t repairId, bool& found) const',
  'fail-closed RepairCosting repair lookup implementation');
requireText(
  load,
  'bool repairFound = false;',
  'explicit repair existence result in load');
requireText(
  load,
  'if (!repairExists(repairId, repairFound) || !repairFound) return false;',
  'repair identity validation owned by RepairCosting::load');
requireText(
  costing,
  'WarehouseMovementIntegrityAudit::checkRepair(m_storage, repairId, wireTotals)',
  'authoritative movement validation retained by load');

// Each RepairCostingWeb operation proves exact repair existence first. Its
// read/preflight costing call must therefore use the known-repair path instead
// of immediately repeating the full repairs.ndjson scan. savePricing() itself
// remains the mutation-time owner of a fresh generic load() validation.
const webKnownLoads = (web.match(/m_costing\.loadKnownRepair\(repairId, /g) || []).length;
if (webKnownLoads !== 3) {
  throw new Error(`RepairCostingWeb must reuse exact repair proof in all three paths; found ${webKnownLoads} known-repair loads`);
}
if (web.includes('m_costing.load(repairId,')) {
  throw new Error('RepairCostingWeb must not repeat repairs.ndjson through generic load() after exact repairExists proof');
}
const webRepairProofs = (web.match(/m_costing\.repairExists\(repairId, repairFound\)/g) || []).length;
if (webRepairProofs !== 3) {
  throw new Error(`RepairCostingWeb must retain exact repair proof in all three paths; found ${webRepairProofs}`);
}
requireText(
  web,
  'if (!m_costing.savePricing(repairId, labour, clientPrice, currency, timestamp))',
  'pricing mutation must still delegate to savePricing');

console.log('Repair pricing single-pass contracts OK: Web read/preflight paths reuse their exact repair proof, while generic load and savePricing retain authoritative repair validation at their safety boundaries.');
