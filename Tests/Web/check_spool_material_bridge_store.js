const fs = require('fs');
const path = require('path');

const root = path.resolve(__dirname, '../..');
const header = fs.readFileSync(path.join(root, 'firmware/esp32/src/CM_SpoolMaterialBridgeStore.h'), 'utf8');
const source = fs.readFileSync(path.join(root, 'firmware/esp32/src/CM_SpoolMaterialBridgeStore.cpp'), 'utf8');
const writeoff = fs.readFileSync(path.join(root, 'firmware/esp32/src/CM_WarehouseWriteOffWeb.cpp'), 'utf8');
const warehouseHeader = fs.readFileSync(path.join(root, 'firmware/esp32/src/CM_WarehouseStore.h'), 'utf8');
const writeoffStore = fs.readFileSync(path.join(root, 'firmware/esp32/src/CM_WarehouseWriteOff.cpp'), 'utf8');
const writeoffRecovery = fs.readFileSync(path.join(root, 'firmware/esp32/src/CM_WarehouseWriteOffRecovery.cpp'), 'utf8');
const runWire = fs.readFileSync(path.join(root, 'firmware/esp32/src/CM_RunWireIssueCoordinator.cpp'), 'utf8');
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
  'bridge.linkedAt.length() >= 10U && bridge.linkedAt.length() <= 32U',
  'FlatJsonObjectValidator::valid(line)',
  'parsed.bridgeId <= previousId',
  'constexpr uint8_t BridgeAuditBatchSize = 24U;',
  'uint32_t batchSpoolIds[BridgeAuditBatchSize]',
  'uint8_t matches[BridgeAuditBatchSize]',
  'if (batchCount == 0U) return true;',
  'batchCount >= BridgeAuditBatchSize',
  'if (batchCount < BridgeAuditBatchSize) return true;'
]) must(source, token, 'bridge store');

mustNot(source, 'RUN_COMPLETED', 'bridge store must not react to run completion');
mustNot(source, 'confirmSpoolWriteOff', 'bridge store must not mutate physical spool stock');
mustNot(source, 'confirmUsage', 'bridge store must not mutate MaterialLedger stock');
mustNot(source, 'adjustMaterial', 'bridge store must not mutate MaterialLedger stock');

for (const token of [
  '"/api/warehouse/write-offs", HTTP_POST',
  'm_server.send(410',
  'legacy_writeoff_post_disabled',
  'write_performed',
  'replacement',
  '/api/material-requests/warehouse',
  '"/api/warehouse/write-offs", HTTP_GET',
  'handleListWriteOffs()'
]) must(writeoff, token, 'legacy writeoff hard deprecation');

// Historical direct journal shape remains private only for startup recovery.
const classStart = warehouseHeader.indexOf('class WarehouseStore');
const privateStart = warehouseHeader.indexOf('\nprivate:', classStart);
const confirmedType = warehouseHeader.indexOf('struct ConfirmedSpoolWriteOff', classStart);
const publicKgFirst = warehouseHeader.indexOf('struct KgFirstWriteOff');
if (classStart < 0 || privateStart < 0 || confirmedType <= privateStart) {
  failures.push('warehouse header: legacy ConfirmedSpoolWriteOff support type must remain private');
}
if (publicKgFirst < 0 || publicKgFirst >= classStart) {
  failures.push('warehouse header: managed RUN_WIRE KgFirstWriteOff type must remain public');
}
for (const token of ['SpoolWriteOffResult', 'confirmSpoolWriteOff(', 'confirmKgFirstWriteOff(']) {
  mustNot(warehouseHeader, token, 'dead direct-writeoff Store API');
  mustNot(writeoffStore, token, 'dead direct-writeoff implementation');
}

// Recovery still closes historical direct PENDING records deterministically;
// the append helper remains internal and no new direct entrypoint is restored.
for (const token of [
  'pending.mode == WarehouseWriteOffMode::LegacySpool',
  'ConfirmedSpoolWriteOff operation;',
  'currentWeight == pending.weightBeforeGrams',
  'currentWeight == pending.weightAfterGrams',
  'appendWriteOffRecord(pending.movementId'
]) must(writeoffRecovery, token, 'legacy pending recovery');
for (const token of [
  'bool WarehouseStore::appendWriteOffRecord',
  'bool WarehouseStore::appendKgFirstWriteOffRecord',
  'bool WarehouseStore::nextMovementId',
  'bool WarehouseStore::rewriteSpoolWeight'
]) must(writeoffStore, token, 'retained internal writeoff helper');

for (const token of [
  'JobSpoolSelectionStore::loadReadOnly',
  'selection.spoolId != spoolId',
  'WindingSessionCompletionAudit::check',
  'm_warehouse.confirmedWriteOffForSourceRun'
]) must(runWire, token, 'atomic RUN_WIRE exact-spool safety');

if (failures.length) {
  console.error(failures.join('\n'));
  process.exit(1);
}
console.log('Spool material bridge foundation contracts OK: public legacy POST and dead direct Store entrypoints are removed, historical recovery helpers remain deterministic, and atomic RUN_WIRE exact-spool/run safety stays authoritative.');
