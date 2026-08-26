const fs = require('fs');
const path = require('path');

const repoRoot = path.resolve(__dirname, '../..');
const webRoot = path.join(repoRoot, 'firmware/esp32/web');
const failures = [];

const desktopCatalog = fs.readFileSync(path.join(webRoot, 'desktop/motors.html'), 'utf8');
const desktopCreate = fs.readFileSync(path.join(webRoot, 'desktop/motor-new.html'), 'utf8');
const mobileCatalog = fs.readFileSync(path.join(webRoot, 'mobile/motors.html'), 'utf8');

for (const field of ['manufacturer', 'model', 'phase_count', 'slot_count', 'coil_program', 'repeat_target']) {
  if (!desktopCreate.includes(`name="${field}"`)) failures.push(`desktop/motor-new.html: missing ${field} field`);
  if (!mobileCatalog.includes(`name="${field}"`)) failures.push(`mobile/motors.html: missing ${field} field`);
}

if (desktopCatalog.includes('id="motorForm"') || desktopCatalog.includes('<h2>Новый двигатель</h2>')) {
  failures.push('desktop/motors.html: catalog must not contain inline motor creation form');
}
if (!desktopCatalog.includes('/desktop/motor-new.html')) failures.push('desktop/motors.html: dedicated create-page link missing');
if (!desktopCatalog.includes('/api/motors/winding/latest')) failures.push('desktop/motors.html: latest winding version lookup missing');
for (const role of ['WORKING', 'STARTING']) {
  if (!desktopCatalog.includes(role)) failures.push(`desktop/motors.html: ${role} catalog column missing`);
}
if (!desktopCatalog.includes('legacy WORKING')) failures.push('desktop/motors.html: legacy winding fallback marker missing');

for (const [relative, source] of [['desktop/motor-new.html', desktopCreate], ['mobile/motors.html', mobileCatalog]]) {
  if (!source.includes('38/38')) failures.push(`${relative}: accepted program/repeat example missing`);
  if (!source.includes('max="65535"')) failures.push(`${relative}: repeat_target limit mismatch`);
  if (!source.includes('физический START') && !source.includes('физического START')) failures.push(`${relative}: per-repeat physical START wording missing`);
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

console.log('Motor Web contracts OK: desktop catalog is read-only, dedicated create page preserves schema/safety, versioned WORKING/STARTING lookup and legacy fallback remain explicit.');
