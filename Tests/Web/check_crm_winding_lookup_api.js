const fs = require('fs');

const header = fs.readFileSync('firmware/esp32/src/CM_RepairRegistryLookupWeb.h', 'utf8');
const source = fs.readFileSync('firmware/esp32/src/CM_RepairRegistryLookupWeb.cpp', 'utf8');
const registryHeader = fs.readFileSync('firmware/esp32/src/CM_RepairRegistry.h', 'utf8');
const registrySource = fs.readFileSync('firmware/esp32/src/CM_RepairRegistry.cpp', 'utf8');
const autonomousSource = fs.readFileSync('firmware/esp32/src/CM_AutonomousWindingWeb.cpp', 'utf8');
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

for (const required of [
  'bool clientExists(uint32_t clientId, bool& found) const;',
  'bool motorExists(uint32_t motorId, bool& found) const;',
  'bool idExists(const char* path, const char* key, uint32_t id, bool& found) const;'
]) must(registryHeader, required, 'fail-closed registry lookup API');
for (const forbidden of [
  'bool clientExists(uint32_t clientId) const;',
  'bool motorExists(uint32_t motorId) const;',
  'bool idExists(const char* path, const char* key, uint32_t id) const;'
]) {
  if (registryHeader.includes(forbidden)) throw new Error('ambiguous registry lookup API returned: ' + forbidden);
}
for (const required of [
  'found = false;',
  'if (!m_storage.exists(path)) return true;',
  'found = matches == 1U;',
  'bool clientFound = false;',
  'bool motorFound = false;',
  '!clientExists(repair.clientId, clientFound) || !clientFound',
  '!motorExists(repair.motorId, motorFound) || !motorFound'
]) must(registrySource, required, 'registry lookup result channel');

for (const required of [
  'bool resolveMotor(RepairRegistry& registry,',
  'if (!registry.motorExists(motorId, found))',
  '"{\\\"error\\\":\\\"motor_lookup_integrity_failed\\\"}"',
  '"{\\\"error\\\":\\\"motor_not_found\\\"}"',
  'if (!resolveMotor(m_registry, m_server, motorId)) return;'
]) must(source, required, 'lookup Web motor failure/not-found split');
for (const required of [
  'bool motorFound = false;',
  'if (!m_registry.motorExists(motorId, motorFound))',
  '"{\\\"error\\\":\\\"motor_lookup_integrity_failed\\\"}"',
  'if (!motorFound)',
  '"{\\\"error\\\":\\\"motor_not_found\\\"}"'
]) must(autonomousSource, required, 'autonomous assignment motor failure/not-found split');

for (const [label, text] of [['registry lookup Web', source], ['autonomous Web', autonomousSource]]) {
  if (/motorExists\(motorId\s*\)/.test(text)) throw new Error(`${label}: ambiguous one-arg motorExists returned`);
}

for (const forbidden of [
  'HTTP_POST,\n                [this]() { handleMotorWinding',
  'HTTP_POST,\n                [this]() { handleRepairAsReceived'
]) {
  if (source.includes(forbidden)) throw new Error('lookup endpoints must remain read-only');
}

console.log('CRM winding/as-received lookup API contract: OK; client/motor existence is fail-closed and Web distinguishes integrity failure from true not-found.');
