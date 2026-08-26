const fs = require('fs');
const path = require('path');

const root = path.resolve(__dirname, '../..');
const header = fs.readFileSync(path.join(root, 'firmware/esp32/src/CM_SpoolMaterialBridgeStore.h'), 'utf8');
const source = fs.readFileSync(path.join(root, 'firmware/esp32/src/CM_SpoolMaterialBridgeStore.cpp'), 'utf8');
const writeoff = fs.readFileSync(path.join(root, 'firmware/esp32/src/CM_WarehouseWriteOffWeb.cpp'), 'utf8');
const failures = [];
const must = (src, token, label) => { if (!src.includes(token)) failures.push(`${label}: missing ${token}`); };
const mustNot = (src, token, label) => { if (src.includes(token)) failures.push(`${label}: forbidden ${token}`); };

for (const token of [
  '/data/warehouse/spool-material-bridges.ndjson',
  'uint32_t spoolId',
  'uint32_t warehouseItemId',
  'String wireType',
  'uint16_t diameterHundredthsMm',
  'bool append(',
  'bool loadBySpool(',
  'bool validateAll() const'
]) must(header, token, 'bridge header');

for (const token of [
  'FILE_APPEND',
  'file.flush()',
  'if (!loadBySpool(source.spoolId, existing, found) || found) return false;',
  'bridge.wireType == "CU" || bridge.wireType == "AL"',
  'linkedAt.length() >= 10U && linkedAt.length() <= 32U',
  'FlatJsonObjectValidator::valid(line)',
  'parsed.bridgeId <= previousId'
]) must(source, token, 'bridge store');

mustNot(source, 'RUN_COMPLETED', 'bridge store must not react to run completion');
mustNot(source, 'confirmSpoolWriteOff', 'bridge store must not mutate physical spool stock');
mustNot(source, 'confirmUsage', 'bridge store must not mutate MaterialLedger stock');
mustNot(source, 'adjustMaterial', 'bridge store must not mutate MaterialLedger stock');

for (const token of [
  'source_session_and_run_required',
  'source_session_spool_mismatch',
  'source_run_not_completed',
  'confirmedWriteOffForSourceRun',
  'spool_id_required_for_kg_first'
]) must(writeoff, token, 'legacy writeoff safety must remain intact');

if (failures.length) {
  console.error(failures.join('\n'));
  process.exit(1);
}
console.log('Spool material bridge foundation contracts OK: append-only identity mapping only; legacy exact-spool writeoff safety remains unchanged.');
