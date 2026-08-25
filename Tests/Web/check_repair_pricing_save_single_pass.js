const fs = require('fs');
const path = require('path');

const repoRoot = path.resolve(__dirname, '..', '..');
const costingPath = path.join(repoRoot, 'firmware', 'esp32', 'src', 'CM_RepairCosting.cpp');
const costing = fs.readFileSync(costingPath, 'utf8');

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

if (savePricing.includes('repairExists(')) {
  throw new Error('savePricing must not perform a duplicate repair identity scan before load()');
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
  costing,
  'if (!ready() || repairId == 0UL || !repairExists(repairId)) return false;',
  'repair identity validation owned by RepairCosting::load');
requireText(
  costing,
  'WarehouseMovementIntegrityAudit::checkRepair(m_storage, repairId, wireTotals)',
  'authoritative movement validation retained by load');

console.log('Repair pricing save single-pass contracts OK: savePricing reuses load() repair identity validation without a duplicate repairs.ndjson scan.');
