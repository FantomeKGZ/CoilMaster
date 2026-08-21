const fs = require('fs');
const path = require('path');

const repoRoot = path.resolve(__dirname, '../..');
const webRoot = path.join(repoRoot, 'firmware/esp32/web');
const failures = [];

for (const relative of ['desktop/motors.html', 'mobile/motors.html']) {
  const source = fs.readFileSync(path.join(webRoot, relative), 'utf8');
  for (const field of ['manufacturer', 'model', 'phase_count', 'slot_count', 'coil_program', 'repeat_target']) {
    if (!source.includes(`name="${field}"`)) failures.push(`${relative}: missing ${field} field`);
  }
  if (!source.includes('не указано')) failures.push(`${relative}: legacy unknown values are not shown explicitly`);
  if (!source.includes('каждый повтор') && !source.includes('Каждый повтор')) failures.push(`${relative}: per-repeat physical START wording missing`);
  if (!source.includes('38/38')) failures.push(`${relative}: accepted program/repeat example missing`);
  if (!source.includes('max="65535"')) failures.push(`${relative}: repeat_target limit mismatch`);
}

const repairQuickAdd = fs.readFileSync(path.join(webRoot, 'desktop/repairs.html'), 'utf8');
for (const field of ['manufacturer', 'model', 'phase_count', 'slot_count', 'coil_program', 'repeat_target']) {
  if (!repairQuickAdd.includes(`name="${field}"`)) failures.push(`desktop/repairs.html: quick-add missing ${field}`);
}
if (!repairQuickAdd.includes('38/38')) failures.push('desktop/repairs.html: quick-add program/repeat example missing');
if (!repairQuickAdd.includes('физический START')) failures.push('desktop/repairs.html: quick-add physical START wording missing');
if (!repairQuickAdd.includes('max="65535"')) failures.push('desktop/repairs.html: quick-add repeat_target limit mismatch');
if (!repairQuickAdd.includes("fd.set('name',derived)")) failures.push('desktop/repairs.html: quick-add legacy name derivation missing');

const registryHeader = fs.readFileSync(path.join(repoRoot, 'firmware/esp32/src/CM_RepairRegistry.h'), 'utf8');
const registrySource = fs.readFileSync(path.join(repoRoot, 'firmware/esp32/src/CM_RepairRegistry.cpp'), 'utf8');
const registryWeb = fs.readFileSync(path.join(repoRoot, 'firmware/esp32/src/CM_RepairRegistryWeb.cpp'), 'utf8');

if (!registryHeader.includes('uint16_t repeatTarget;')) failures.push('CM_RepairRegistry.h: repeatTarget storage field missing');
if (!registryHeader.includes('repeatTarget(1U)')) failures.push('CM_RepairRegistry.h: new motor repeat default must be 1');
if (!registrySource.includes('repeat_target')) failures.push('CM_RepairRegistry.cpp: repeat_target is not persisted');
if (!registryWeb.includes('"phase_count"')) failures.push('CM_RepairRegistryWeb.cpp: phase_count API alias missing');
if (!registryWeb.includes('"repeat_target"')) failures.push('CM_RepairRegistryWeb.cpp: repeat_target API field missing');
if (!registryWeb.includes('0xFFFFUL')) failures.push('CM_RepairRegistryWeb.cpp: repeat_target uint16 limit missing');
if (!registryWeb.includes('conflicting_phase_count')) failures.push('CM_RepairRegistryWeb.cpp: phase_count/phases conflict guard missing');

if (failures.length) {
  console.error(failures.join('\n'));
  process.exit(1);
}

console.log('Motor schema UI contracts OK: catalogs and repair quick-add expose manufacturer/model, phase_count, slot_count, program, repeat_target, legacy compatibility, and physical START semantics.');
