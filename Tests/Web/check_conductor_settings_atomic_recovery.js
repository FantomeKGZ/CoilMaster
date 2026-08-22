const fs = require('fs');
const path = require('path');

const root = path.resolve(__dirname, '../..');
const sourcePath = 'firmware/esp32/src/CM_ConductorSettings.cpp';
const source = fs.readFileSync(path.join(root, sourcePath), 'utf8');
const failures = [];

function requireText(text, message) {
  if (!source.includes(text)) failures.push(sourcePath + ': ' + message);
}

requireText('const bool backupValid = backupExists && loadFromPath(BackupPath, backupSettings);',
  'backup validity must be checked before recovery choice');
requireText('if (backupExists)\n    {\n        if (!backupValid) return false;',
  'existing backup must remain authoritative or fail closed');
requireText('if (tempExists && !m_storage.remove(TempPath)) return false;\n        return m_storage.rename(BackupPath, SettingsPath);',
  'prepared temp must be discarded before committed backup restore');
requireText('if (tempValid) return m_storage.rename(TempPath, SettingsPath);',
  'temp promotion must be restricted to no-backup first write');

const backupBranch = source.indexOf('if (backupExists)\n    {\n        if (!backupValid) return false;');
const tempPromotion = source.indexOf('if (tempValid) return m_storage.rename(TempPath, SettingsPath);');
if (backupBranch < 0 || tempPromotion < 0 || backupBranch >= tempPromotion)
  failures.push(sourcePath + ': committed backup must be evaluated before temp promotion');

const oldUnsafe = 'if (tempValid)\n    {\n        if (!m_storage.rename(TempPath, SettingsPath)) return false;\n        if (backupExists && !m_storage.remove(BackupPath)) return false;';
if (source.includes(oldUnsafe))
  failures.push(sourcePath + ': uncommitted conductor temp still wins over committed backup');

if (failures.length) {
  console.error(failures.join('\n'));
  process.exit(1);
}
console.log('Conductor settings atomic recovery OK: committed backup wins over prepared temp; temp promotion is first-write only.');
