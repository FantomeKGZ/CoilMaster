const fs = require('fs');
const path = require('path');

const repoRoot = path.resolve(__dirname, '../..');
const webRoot = path.join(repoRoot, 'firmware/esp32/web');
const failures = [];

for (const relative of ['desktop/motor-details.html', 'mobile/motor-details.html']) {
  const source = fs.readFileSync(path.join(webRoot, relative), 'utf8');
  for (const token of ['/api/motors/by-id', '/api/motors/repairs', 'repeat_target', 'slot_count', 'manufacturer', 'model', 'не указано']) {
    if (!source.includes(token)) failures.push(`${relative}: missing ${token}`);
  }
  if (!source.includes('физического START') && !source.includes('физический START')) {
    failures.push(`${relative}: physical START repeat safety wording missing`);
  }
  if (!source.includes('winding-history.html?repair_id=')) failures.push(`${relative}: winding history link missing`);
  if (!source.includes('costing.html?repair_id=')) failures.push(`${relative}: repair costing link missing`);
  if (!source.includes('has_more') || !source.includes('next_cursor')) failures.push(`${relative}: bounded repair history paging missing`);
}

for (const relative of ['desktop/motors.html', 'mobile/motors.html']) {
  const source = fs.readFileSync(path.join(webRoot, relative), 'utf8');
  if (!source.includes('motor-details.html?motor_id=')) failures.push(`${relative}: motor details link missing`);
}

const registryHeader = fs.readFileSync(path.join(repoRoot, 'firmware/esp32/src/CM_RepairRegistry.h'), 'utf8');
const registryPage = fs.readFileSync(path.join(repoRoot, 'firmware/esp32/src/CM_RepairRegistryPage.cpp'), 'utf8');
const lookupHeader = fs.readFileSync(path.join(repoRoot, 'firmware/esp32/src/CM_RepairRegistryLookupWeb.h'), 'utf8');
const lookupSource = fs.readFileSync(path.join(repoRoot, 'firmware/esp32/src/CM_RepairRegistryLookupWeb.cpp'), 'utf8');

if (!registryHeader.includes('uint32_t motorId')) failures.push('CM_RepairRegistry.h: motorId repair-page filter missing');
if (!registryHeader.includes('0UL,\n                                     statusFilter')) failures.push('CM_RepairRegistry.h: backward-compatible repair paging overload missing');
if (!registryPage.includes('lineMotorId != motorId')) failures.push('CM_RepairRegistryPage.cpp: exact motor_id repair filter missing');
if (!lookupHeader.includes('handleMotorRepairs')) failures.push('CM_RepairRegistryLookupWeb.h: motor repair handler missing');
if (!lookupSource.includes('/api/motors/repairs')) failures.push('CM_RepairRegistryLookupWeb.cpp: motor repair endpoint missing');
if (!lookupSource.includes('appendRepairsPageJson(response,\n                                          0UL,\n                                          motorId')) failures.push('CM_RepairRegistryLookupWeb.cpp: endpoint does not use exact motor_id paging');
if (!lookupSource.includes('MaxListPageSize')) failures.push('CM_RepairRegistryLookupWeb.cpp: motor repair endpoint is not bounded');

if (failures.length) {
  console.error(failures.join('\n'));
  process.exit(1);
}

console.log('Motor details contracts OK: exact lookup, bounded motor repair history, legacy-safe unknown display, catalog links, and physical START semantics are present.');
