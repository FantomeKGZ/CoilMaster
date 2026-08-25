const fs = require('fs');

const header = fs.readFileSync('firmware/esp32/src/CM_MaterialRequestWarehousePendingStore.h', 'utf8');
const cpp = fs.readFileSync('firmware/esp32/src/CM_MaterialRequestWarehousePendingStore.cpp', 'utf8');

function requireText(source, text, label) {
  if (!source.includes(text)) throw new Error(`missing ${label}: ${text}`);
}

requireText(header,
  'material-request-warehouse.pending.json',
  'durable pending marker');
requireText(header,
  'material-request-warehouse.pending.tmp',
  'atomic temp marker');
requireText(cpp,
  'if (!ready() || !pending.valid() || m_storage.exists(Path)) return false;',
  'single in-flight transaction guard');
requireText(cpp,
  'return m_storage.rename(TempPath, Path);',
  'atomic temp-to-main publish');
requireText(cpp,
  'movementKind == "CORRECTION"',
  'correction contract');
requireText(cpp,
  'correctionDirection != "ADD" && correctionDirection != "REMOVE"',
  'correction direction validation');
requireText(cpp,
  'sourceKind == "RUN_WIRE"',
  'run-linked wire branch');
requireText(cpp,
  'sourceSessionId > 0UL && sourceRunId > 0UL',
  'exact run provenance');
requireText(cpp,
  'movementKind == "ISSUE" && unit == "KG"',
  'RUN_WIRE issue-only kg rule');
requireText(cpp,
  'verified.transactionRef != pending.transactionRef',
  'read-after-write verification');

console.log('Material Request warehouse pending transaction contracts OK');
