const fs = require('fs');
const path = require('path');

const repoRoot = path.resolve(__dirname, '../..');
const webRoot = path.join(repoRoot, 'firmware/esp32/web');
const failures = [];

const desktopCatalog = fs.readFileSync(path.join(webRoot, 'desktop/motors.html'), 'utf8');
const desktopCreate = fs.readFileSync(path.join(webRoot, 'desktop/motor-new.html'), 'utf8');
const mobileCatalog = fs.readFileSync(path.join(webRoot, 'mobile/motors.html'), 'utf8');
const mobileCreate = fs.readFileSync(path.join(webRoot, 'mobile/motor-new.html'), 'utf8');

for (const [relative, source] of [
  ['desktop/motor-new.html', desktopCreate],
  ['mobile/motor-new.html', mobileCreate]
]) {
  for (const field of ['manufacturer', 'model', 'phase_count', 'slot_count', 'coil_program', 'repeat_target']) {
    if (!source.includes(`name="${field}"`)) failures.push(`${relative}: missing ${field} field`);
  }
  if (!source.includes('38/38')) failures.push(`${relative}: accepted program/repeat example missing`);
  if (!source.includes('max="65535"')) failures.push(`${relative}: repeat_target limit mismatch`);
  if (!source.includes('START')) failures.push(`${relative}: local physical START wording missing`);
}

for (const [relative, source, createPath, repairPath] of [
  ['desktop/motors.html', desktopCatalog, '/desktop/motor-new.html', '/desktop/repair-new.html'],
  ['mobile/motors.html', mobileCatalog, '/mobile/motor-new.html', '/mobile/repair-new.html']
]) {
  if (source.includes('id="motorForm"') || source.includes("fetch('/api/motors',{method:'POST'")) {
    failures.push(`${relative}: catalog must not contain inline motor creation`);
  }
  if (!source.includes(createPath)) failures.push(`${relative}: dedicated create-page link missing`);
  if (!source.includes(repairPath)) failures.push(`${relative}: dedicated repair-page link missing`);
  if (!source.includes('motor-details.html?motor_id=')) failures.push(`${relative}: motor details link missing`);
}

if (!desktopCatalog.includes('/api/motors/winding/latest')) failures.push('desktop/motors.html: latest winding version lookup missing');
for (const role of ['Рабочая обмотка', 'Пусковая обмотка']) {
  if (!desktopCatalog.includes(role)) failures.push(`desktop/motors.html: ${role} catalog column missing`);
}
if (!desktopCatalog.includes('legacy: рабочая обмотка')) failures.push('desktop/motors.html: legacy winding fallback marker missing');

const repairCatalog = fs.readFileSync(path.join(webRoot, 'desktop/repairs.html'), 'utf8');
if (!repairCatalog.includes('/desktop/motor-new.html')) failures.push('desktop/repairs.html: dedicated motor-create link missing');
if (!repairCatalog.includes('/desktop/repair-new.html')) failures.push('desktop/repairs.html: dedicated repair-create link missing');
if (repairCatalog.includes('name="coil_program"') || repairCatalog.includes("fetch('/api/motors',{method:'POST'")) {
  failures.push('desktop/repairs.html: legacy inline motor quick-add must stay removed');
}

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

console.log('Motor Web contracts OK: catalogs are read-only, dedicated desktop/mobile creation preserves schema/safety, and versioned winding lookup plus legacy fallback stay explicit.');
