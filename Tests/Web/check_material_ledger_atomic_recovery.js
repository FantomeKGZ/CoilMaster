const fs = require('fs');

const source = fs.readFileSync('firmware/esp32/src/CM_MaterialLedgerSwap.cpp', 'utf8');
const adjustment = fs.readFileSync('firmware/esp32/src/CM_MaterialAdjustment.cpp', 'utf8');
const header = fs.readFileSync('firmware/esp32/src/CM_MaterialLedger.h', 'utf8');

function requireText(text, needle, message) {
  if (!text.includes(needle)) throw new Error(message);
}
function forbidText(text, needle, message) {
  if (text.includes(needle)) throw new Error(message);
}

requireText(header, 'bool validateMaterialsFile(const char* path) const;',
  'material ledger swaps need a shared parser-backed validator');
requireText(source, '!validateMaterialsFile(MaterialsPath) ||\n        !validateMaterialsFile(MaterialsTempPath)',
  'both committed main and prepared temp must be valid before replacement');
requireText(source, 'if (!validateMaterialsFile(MaterialsBackupPath)) return false;',
  'last committed material backup must be validated before recovery decisions');
requireText(source, 'if (validateMaterialsFile(MaterialsPath))\n            return m_storage.remove(MaterialsBackupPath);',
  'backup cleanup must happen only after replacement main validates');
requireText(source, 'if (!validateMaterialsFile(MaterialsPath))\n    {',
  'committed material pathname must be revalidated after temp rename');
requireText(source, 'm_storage.rename(MaterialsBackupPath, MaterialsPath)',
  'damaged replacement must have a rollback path to committed material backup');
requireText(source, 'return !hasMain || validateMaterialsFile(MaterialsPath);',
  'material ledger begin must fail closed on corrupt authoritative inventory');

const backupValidation = source.indexOf('if (!validateMaterialsFile(MaterialsBackupPath)) return false;');
const backupRemoval = source.indexOf('return m_storage.remove(MaterialsBackupPath);');
if (backupValidation < 0 || backupRemoval < 0 || backupValidation > backupRemoval) {
  throw new Error('material backup must be validated before it can be discarded');
}

forbidText(header, 'bool adjustmentExists(uint32_t adjustmentId) const;',
  'dead adjustment existence helper declaration returned');
forbidText(adjustment, 'bool MaterialLedger::adjustmentExists(uint32_t adjustmentId) const',
  'dead adjustment existence full-log scan returned');
for (const token of [
  'bool MaterialLedger::recoverPendingAdjustment()',
  'if (durable) return m_storage.remove(AdjustmentPendingPath);',
  'if (beforeState) return m_storage.remove(AdjustmentPendingPath);',
  'if (!afterState) return false;',
  'if (!appendAdjustmentLine(auditLine)) return false;'
]) {
  requireText(adjustment, token, `adjustment recovery invariant missing after dead-helper cleanup: ${token}`);
}

console.log('Material ledger atomic recovery contracts OK: material swap recovery stays fail-closed and the unused adjustmentExists full-log helper remains removed.');
