const fs = require('fs');
const path = require('path');

const root = path.resolve(__dirname, '../..');
const header = fs.readFileSync(path.join(root, 'firmware/esp32/src/CM_SpoolMaterialBridgeStore.h'), 'utf8');
const source = fs.readFileSync(path.join(root, 'firmware/esp32/src/CM_SpoolMaterialBridgeStore.cpp'), 'utf8');
const writeoff = fs.readFileSync(path.join(root, 'firmware/esp32/src/CM_WarehouseWriteOffWeb.cpp'), 'utf8');
const warehouseHeader = fs.readFileSync(path.join(root, 'firmware/esp32/src/CM_WarehouseStore.h'), 'utf8');
const warehouseStore = fs.readFileSync(path.join(root, 'firmware/esp32/src/CM_WarehouseStore.cpp'), 'utf8');
const movementAuditHeader = fs.readFileSync(path.join(root, 'firmware/esp32/src/CM_WarehouseMovementIntegrityAudit.h'), 'utf8');
const movementAudit = fs.readFileSync(path.join(root, 'firmware/esp32/src/CM_WarehouseMovementIntegrityAudit.cpp'), 'utf8');
const repairValidation = fs.readFileSync(path.join(root, 'firmware/esp32/src/CM_WarehouseRepairValidation.cpp'), 'utf8');
const spoolIdentity = fs.readFileSync(path.join(root, 'firmware/esp32/src/CM_WarehouseSpoolIdentity.cpp'), 'utf8');
const materialCatalogue = fs.readFileSync(path.join(root, 'firmware/esp32/src/CM_WarehouseMaterialCatalogue.cpp'), 'utf8');
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
  'analyzeBySpool(source.spoolId, existing, found, bridgeId)',
  'bridge.wireType == "CU" || bridge.wireType == "AL"',
  'bridge.linkedAt.length() >= 10U && bridge.linkedAt.length() <= 32U',
  'FlatJsonObjectValidator::valid(line)',
  'parsed.bridgeId <= previousId',
  'nextBridgeId = previousId + 1UL;',
  'constexpr uint8_t BridgeAuditBatchSize = 24U;',
  'uint32_t batchSpoolIds[BridgeAuditBatchSize]',
  'uint8_t matches[BridgeAuditBatchSize]',
  'if (batchCount == 0U) return true;',
  'batchCount >= BridgeAuditBatchSize',
  'if (batchCount < BridgeAuditBatchSize) return true;'
]) must(source, token, 'bridge store');

mustNot(source, 'bool SpoolMaterialBridgeStore::nextBridgeId(', 'retired second bridge append scan');
const appendStart = source.indexOf('bool SpoolMaterialBridgeStore::append(');
const loadStart = source.indexOf('bool SpoolMaterialBridgeStore::loadBySpool(', appendStart);
const analyzeStart = source.indexOf('bool SpoolMaterialBridgeStore::analyzeBySpool(', loadStart);
const validateStart = source.indexOf('bool SpoolMaterialBridgeStore::validateAll()', analyzeStart);
if (appendStart < 0 || loadStart < 0 || analyzeStart < 0 || validateStart < 0) {
  failures.push('bridge store: single-pass append boundaries missing');
} else {
  const appendBody = source.slice(appendStart, loadStart);
  mustNot(appendBody, 'loadBySpool(', 'append must use shared analyzer directly');
  mustNot(appendBody, 'nextBridgeId(', 'append must not perform a second id scan');
  const analyzeBody = source.slice(analyzeStart, validateStart);
  const opens = (analyzeBody.match(/m_storage\.open\(Path, FILE_READ\)/g) || []).length;
  if (opens !== 1) failures.push(`bridge store: append analyzer must open bridge log once, found ${opens}`);
  must(analyzeBody, 'if (found)', 'single-pass duplicate target spool rejection');
  must(analyzeBody, 'if (previousId == 0xFFFFFFFFUL) return false;', 'single-pass bridge id overflow guard');
}

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

must(warehouseHeader, 'bool repairExists(uint32_t repairId,bool& found) const;', 'fail-closed repair lookup API');
must(repairValidation, 'bool WarehouseStore::repairExists(uint32_t repairId, bool& found) const', 'fail-closed repair lookup implementation');
mustNot(warehouseHeader, 'bool repairExists(uint32_t repairId) const;', 'ambiguous repair lookup wrapper');
mustNot(repairValidation, 'bool WarehouseStore::repairExists(uint32_t repairId) const', 'ambiguous repair lookup implementation');

must(warehouseHeader, 'bool loadActiveSpoolIdentity(uint32_t spoolId,ActiveWireSpoolIdentity& identity,bool& found) const;', 'fail-closed spool identity API');
must(spoolIdentity, 'ActiveWireSpoolIdentity& identity,\n                                              bool& found) const', 'fail-closed spool identity implementation');
mustNot(warehouseHeader, 'bool loadActiveSpoolIdentity(uint32_t spoolId,ActiveWireSpoolIdentity& identity) const;', 'ambiguous spool identity wrapper');
mustNot(spoolIdentity, 'ActiveWireSpoolIdentity& identity) const', 'ambiguous spool identity implementation');

must(warehouseHeader, 'bool loadKnownWireDiameters(const char* wireType,KnownWireDiameter* items,uint8_t capacity,uint8_t& count) const;', 'fail-closed wire catalogue API');
must(materialCatalogue, 'uint8_t capacity,\n                                             uint8_t& count) const', 'fail-closed wire catalogue implementation');
mustNot(warehouseHeader, 'uint8_t loadKnownWireDiameters(const char* wireType,KnownWireDiameter* items,uint8_t capacity) const;', 'count-only wire catalogue wrapper');
mustNot(materialCatalogue, 'uint8_t WarehouseStore::loadKnownWireDiameters', 'count-only wire catalogue implementation');

const publicSurface = warehouseHeader.slice(classStart, privateStart);
must(publicSurface, 'bool loadWarehousePrice(WarehousePrice& price,bool& configured) const;', 'public explicit price lookup');
mustNot(warehouseHeader, 'bool loadWarehousePrice(WarehousePrice& price) const;', 'retired ambiguous price lookup declaration');
mustNot(warehouseStore, 'bool WarehouseStore::loadWarehousePrice(WarehousePrice& price) const', 'retired ambiguous price lookup implementation');

// Warehouse summary must reuse the authoritative movement codec/integrity pass.
for (const token of [
  'struct WarehouseMovementSummaryTotals',
  'WarehouseMovementDiameterTotals diameters[WarehouseMovementSummaryMaxDiameters]',
  'static bool checkSummary(fs::FS& storage,'
]) must(movementAuditHeader, token, 'audited summary API');
for (const token of [
  'accumulateSummaryRecord(record, monthPrefix, *summaryTotals)',
  'WarehouseWriteOffRecordCodec::parse(line, record)',
  'confirmedProvenanceUnique(storage, Path)',
  'bool WarehouseMovementIntegrityAudit::checkSummary('
]) must(movementAudit, token, 'audited summary implementation');
for (const token of [
  'WarehouseMovementSummaryTotals movementTotals;',
  'WarehouseMovementIntegrityAudit::checkSummary(',
  'summary->consumedMonthGrams = movement.consumedMonthGrams;',
  'summary->consumedAllTimeGrams = movement.consumedAllTimeGrams;'
]) must(warehouseStore, token, 'single-pass warehouse summary integration');
mustNot(warehouseStore, 'WarehouseMovementIntegrityAudit::check(m_storage)', 'duplicate pre-summary full movement audit');
mustNot(warehouseStore, '!FlatJsonObjectValidator::valid(line) ||\n            !findUnsigned(line, "movement_id"', 'retired manual movement parser');

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
console.log('Spool material bridge foundation contracts OK: append duplicate-spool detection and next bridge id now share one validated pass; fail-closed warehouse lookups, deterministic recovery, and atomic RUN_WIRE safety remain authoritative.');
