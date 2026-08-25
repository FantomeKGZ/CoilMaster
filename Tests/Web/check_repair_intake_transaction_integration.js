const fs = require('fs');

const webHeader = fs.readFileSync('firmware/esp32/src/CM_RepairRegistryWeb.h', 'utf8');
const webSource = fs.readFileSync('firmware/esp32/src/CM_RepairRegistryWeb.cpp', 'utf8');
const coordinator = fs.readFileSync('firmware/esp32/src/CM_RepairIntakeCoordinator.cpp', 'utf8');

function must(text, needle, label) {
  if (!text.includes(needle)) throw new Error(`missing ${label}: ${needle}`);
}

must(webHeader, 'RepairIntakeCoordinator* m_intake;', 'repair intake coordinator member');
must(webSource, 'static RepairIntakeCoordinator repairIntake(SD, m_registry);', 'coordinator runtime ownership');
must(webSource, 'm_intake = &repairIntake;', 'coordinator wiring');
must(webSource, 'm_intake->begin();', 'startup recovery before HTTP operation');

const createStart = webSource.indexOf('void RepairRegistryWeb::handleCreateRepair()');
const createEnd = webSource.indexOf('void RepairRegistryWeb::handleRepairFinalization()', createStart);
if (createStart < 0 || createEnd < 0 || createEnd <= createStart) {
  throw new Error('unable to isolate handleCreateRepair');
}
const createBody = webSource.slice(createStart, createEnd);
must(createBody, 'm_intake->ready()', 'repair intake readiness gate');
must(createBody, 'm_intake->create(repair, repairId)', 'transactional repair creation');
must(createBody, 'repair_intake_pending_recovery', 'pending recovery error semantics');
must(createBody, 'repair_intake_integrity_failed', 'fail-closed integrity error semantics');
if (createBody.includes('m_registry.addRepair(')) {
  throw new Error('POST /api/repairs must not bypass RepairIntakeCoordinator');
}

const save = coordinator.indexOf('m_pending.save(pending)');
const addRepair = coordinator.indexOf('m_registry.addRepair(repair, actualRepairId)');
const snapshot = coordinator.indexOf('m_snapshots.append(snapshot, snapshotId)');
const verify = coordinator.indexOf('snapshotMatchesPending(actualRepairId, pending, snapshotFound)');
const clear = coordinator.indexOf('m_pending.clear()', verify);
if ([save, addRepair, snapshot, verify, clear].some(v => v < 0)) {
  throw new Error('transaction sequence markers missing');
}
if (!(save < addRepair && addRepair < snapshot && snapshot < verify && verify < clear)) {
  throw new Error('repair intake transaction ordering changed');
}

must(coordinator, 'if (!recoverPending()) return false;', 'startup recovery gate');
must(coordinator, 'if (!repairFound)', 'uncommitted pending recovery');
must(coordinator, 'if (!snapshotFound)', 'committed repair snapshot recovery');
must(coordinator, 'return m_pending.clear();', 'stale marker cleanup');
must(coordinator, 'pending.sourceKind = versionFound ? F("VERSIONED") : F("LEGACY_MOTOR")', 'legacy/versioned source split');
must(coordinator, 'appendByVersionIdJson', 'exact winding version reconstruction');

console.log('repair intake transaction integration contract: OK');
