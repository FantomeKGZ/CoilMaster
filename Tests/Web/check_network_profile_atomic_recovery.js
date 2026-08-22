'use strict';

const fs = require('fs');
const source = fs.readFileSync('firmware/esp32/src/CM_NetworkProfileStore.cpp', 'utf8');

function requireText(text, message) {
  if (!source.includes(text)) throw new Error(message || `missing ${text}`);
}

requireText('if (mainExists && loadFromPath(ProfilesPath, profiles, count))',
  'valid committed main must remain first recovery authority');
requireText('if (backupValid)',
  'valid backup recovery branch missing');
requireText('if (tempExists && !m_storage.remove(TempPath)) return false;',
  'prepared temp must be discarded when committed backup is restored');
requireText('return m_storage.rename(BackupPath, ProfilesPath);',
  'valid backup must restore the committed profile set');
requireText('if (backupExists) return false;',
  'invalid backup evidence must fail closed instead of being erased');
requireText('if (tempValid)',
  'validated temp recovery for first-write interruption missing');
requireText('return m_storage.rename(TempPath, ProfilesPath);',
  'temp may only become authoritative when no prior backup exists');

const backupBranch = source.indexOf('if (backupValid)');
const invalidBackupGuard = source.indexOf('if (backupExists) return false;', backupBranch);
const tempBranch = source.indexOf('if (tempValid)', invalidBackupGuard);
if (backupBranch < 0 || invalidBackupGuard < 0 || tempBranch < 0 ||
    !(backupBranch < invalidBackupGuard && invalidBackupGuard < tempBranch)) {
  throw new Error('network recovery order must prefer committed backup before candidate temp');
}

console.log('Network profile atomic recovery contracts: OK');
