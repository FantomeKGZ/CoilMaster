const fs = require('fs');

const source = fs.readFileSync('firmware/esp32/src/CM_WarehouseSpoolSwap.cpp', 'utf8');
const header = fs.readFileSync('firmware/esp32/src/CM_WarehouseStore.h', 'utf8');
const writeoff = fs.readFileSync('firmware/esp32/src/CM_WarehouseWriteOff.cpp', 'utf8');
const managed = fs.readFileSync('firmware/esp32/src/CM_WarehouseRunWireManaged.cpp', 'utf8');

function requireText(text, needle, message) {
  if (!text.includes(needle)) throw new Error(message);
}

requireText(header, 'bool validateSpoolsFile(const char* path) const;',
  'warehouse spool swaps need a shared parser-backed validator');
requireText(source, '!validateSpoolsFile(SpoolsPath) ||\n        !validateSpoolsFile(SpoolsTempPath)',
  'both committed main and prepared temp must be valid before replacement');
requireText(source, 'if (!validateSpoolsFile(SpoolsBackupPath)) return false;',
  'last committed spool backup must be validated before recovery decisions');
requireText(source, 'if (validateSpoolsFile(SpoolsPath))\n            return m_storage.remove(SpoolsBackupPath);',
  'backup cleanup must happen only after replacement main validates');
requireText(source, 'if (!validateSpoolsFile(SpoolsPath))\n    {',
  'committed pathname must be revalidated after temp rename');
requireText(source, 'm_storage.rename(SpoolsBackupPath, SpoolsPath)',
  'damaged replacement must have a rollback path to committed backup');
requireText(source, 'return !hasMain || validateSpoolsFile(SpoolsPath);',
  'warehouse begin must fail closed on corrupt authoritative spool inventory');

const backupValidation = source.indexOf('if (!validateSpoolsFile(SpoolsBackupPath)) return false;');
const backupRemoval = source.indexOf('return m_storage.remove(SpoolsBackupPath);');
if (backupValidation < 0 || backupRemoval < 0 || backupValidation > backupRemoval) {
  throw new Error('spool backup must be validated before it can be discarded');
}

const applyStart = managed.indexOf('bool WarehouseStore::applyManagedRunWireSpoolWeight(');
const confirmStart = managed.indexOf('bool WarehouseStore::confirmManagedRunWireWriteOff(', applyStart);
if (applyStart < 0 || confirmStart < 0) {
  throw new Error('managed RUN_WIRE spool mutation boundary is missing');
}
const applyBody = managed.slice(applyStart, confirmStart);
if (applyBody.includes('loadActiveSpoolIdentity(')) {
  throw new Error('managed RUN_WIRE mutation must not pre-scan spools before the checked rewrite');
}
for (const text of [
  'rewriteSpoolWeight(spoolId,',
  'diameterHundredthsMm,',
  'wireType,',
  'true,',
  'resolvedDiameter,',
  'resolvedWireType)'
]) {
  requireText(applyBody, text, `managed single-pass mutation contract ${text}`);
}

for (const text of [
  'const bool atBefore = currentWeight == expectedWeightGrams;',
  'const bool atAfter = allowAlreadyApplied && currentWeight == newWeightGrams;',
  'if (!atBefore && !atAfter)',
  'expectedDiameterHundredthsMm != 0U',
  'currentWireType != expectedWireType',
  'const char* optionalFields[]',
  'while (source.available())',
  'if (alreadyApplied)',
  'return m_storage.remove(SpoolsTempPath);',
  'return replaceSpoolsFileFromTemp();'
]) {
  requireText(writeoff, text, `checked RUN_WIRE spool rewrite contract ${text}`);
}

console.log('Warehouse spool atomic recovery contracts OK; managed RUN_WIRE mutation validates exact before/after identity in the rewrite pass without a redundant spool pre-scan');
