const fs = require('fs');

const header = fs.readFileSync('firmware/esp32/src/CM_RepairIntakePendingStore.h', 'utf8');
const source = fs.readFileSync('firmware/esp32/src/CM_RepairIntakePendingStore.cpp', 'utf8');

function must(text, needle, label) {
  if (!text.includes(needle)) throw new Error(`missing ${label}: ${needle}`);
}

must(header, '/data/workshop/repair-intake.pending.json', 'pending path');
must(header, '/data/workshop/repair-intake.pending.tmp', 'temp path');
must(header, 'uint32_t repairId;', 'repair identity');
must(header, 'uint32_t clientId;', 'client identity');
must(header, 'uint32_t motorId;', 'motor identity');
must(header, 'uint32_t sourceWindingVersionId;', 'source winding identity');
must(header, 'bool hasPending() const;', 'pending probe');
must(header, 'bool save(const RepairIntakePending& pending);', 'durable prepare');
must(header, 'bool clear();', 'commit cleanup');

must(source, 'm_storage.open(TempPath, FILE_WRITE)', 'prepare temp write');
must(source, 'loadPath(TempPath, verified)', 'temp verification before promote');
must(source, 'm_storage.rename(TempPath, Path)', 'atomic promotion');
must(source, 'if (mainExists && loadPath(Path, mainPending))', 'valid main wins recovery');
must(source, 'return m_storage.remove(TempPath);', 'stale temp cleanup');
must(source, 'if (mainExists) return false;', 'invalid ambiguous main fail closed');
must(source, 'if (!loadPath(TempPath, tempPending)) return false;', 'invalid temp fail closed');
must(source, 'if (m_storage.exists(Path)) return false;', 'single pending transaction guard');

if (source.includes('m_storage.remove(Path);\n    return save(')) {
  throw new Error('pending marker must not silently replace an active transaction');
}

console.log('repair intake pending transaction contract: OK');
