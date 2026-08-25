const fs = require('fs');

const header = fs.readFileSync('firmware/esp32/src/CM_RepairRegistryLookupWeb.h', 'utf8');
const source = fs.readFileSync('firmware/esp32/src/CM_RepairRegistryLookupWeb.cpp', 'utf8');
const storeHeader = fs.readFileSync('firmware/esp32/src/CM_MotorWindingVersionStore.h', 'utf8');
const storeSource = fs.readFileSync('firmware/esp32/src/CM_MotorWindingVersionStore.cpp', 'utf8');

function must(text, needle, label) {
  if (!text.includes(needle)) throw new Error(`missing ${label}: ${needle}`);
}

must(header, 'MotorWindingVersionStore m_windingVersions;', 'winding store lifecycle');
must(header, 'RepairAsReceivedSnapshotStore m_asReceivedSnapshots;', 'snapshot store lifecycle');
must(source, 'm_windingVersionsReady = m_windingVersions.begin();', 'winding begin');
must(source, 'm_asReceivedSnapshotsReady = m_asReceivedSnapshots.begin();', 'snapshot begin');
must(source, '/api/motors/winding/latest', 'latest winding endpoint');
must(source, '/api/motors/winding/versions', 'winding versions endpoint');
must(source, '/api/repairs/as-received', 'as-received endpoint');
must(source, 'legacy_motor_fallback_required', 'legacy motor fallback contract');
must(source, 'legacy_repair_without_snapshot', 'legacy repair fallback contract');
must(storeHeader, 'appendLatestByMotorJson', 'latest read method');
must(storeHeader, 'appendMotorPageJson', 'paged read method');
must(storeSource, 'versionId <= cursor', 'monotonic cursor filter');
must(storeSource, 'currentMotorId != motorId', 'exact motor filter');
must(storeSource, 'if (count >= limit)', 'bounded page limit');

for (const forbidden of [
  'HTTP_POST,\n                [this]() { handleMotorWinding',
  'HTTP_POST,\n                [this]() { handleRepairAsReceived'
]) {
  if (source.includes(forbidden)) throw new Error('lookup endpoints must remain read-only');
}

console.log('CRM winding/as-received lookup API contract: OK');
