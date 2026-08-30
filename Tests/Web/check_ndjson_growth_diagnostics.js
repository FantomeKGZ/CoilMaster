const fs = require('fs');
const path = require('path');

const root = path.resolve(__dirname, '../..');
const failures = [];
const backendPath = 'firmware/esp32/src/CM_StorageDiagnosticsWeb.cpp';
const uiPath = 'firmware/esp32/web/shared/settings-system-diagnostics.js';
const backend = fs.readFileSync(path.join(root, backendPath), 'utf8');
const ui = fs.readFileSync(path.join(root, uiPath), 'utf8');

function requireText(relative, source, text, description) {
  if (!source.includes(text)) failures.push(relative + ': ' + description);
}

for (const text of [
  '/data/warehouse/movements.ndjson',
  '/data/winding-runs/events.ndjson',
  '/data/workshop/repairs.ndjson',
  '/data/warehouse/spools.ndjson',
  'warehouse_movements_bytes',
  'winding_events_bytes',
  'repair_registry_bytes',
  'wire_spools_bytes',
  'ndjson_growth_monitoring_only',
  'automatic_cleanup_allowed'
]) {
  requireText(backendPath, backend, text, 'read-only NDJSON growth telemetry missing: ' + text);
}

for (const forbidden of ['.remove(', '.rename(', 'FILE_WRITE', 'FILE_APPEND']) {
  if (backend.includes(forbidden)) failures.push(backendPath + ': storage diagnostics must remain read-only: ' + forbidden);
}

for (const text of [
  "addRow('Журнал списаний',byteSize(storage.warehouse_movements_bytes))",
  "addRow('Журнал намоток',byteSize(storage.winding_events_bytes))",
  "addRow('Реестр ремонтов',byteSize(storage.repair_registry_bytes))",
  "addRow('Реестр бухт',byteSize(storage.wire_spools_bytes))",
  'автоматическая очистка и ротация рабочих данных отключены'
]) {
  requireText(uiPath, ui, text, 'NDJSON growth diagnostics UI missing: ' + text);
}

if (failures.length) {
  console.error(failures.join('\n'));
  process.exit(1);
}

console.log('NDJSON growth diagnostics OK: critical append-oriented file sizes are visible and diagnostics remain read-only with no automatic cleanup or rotation.');
