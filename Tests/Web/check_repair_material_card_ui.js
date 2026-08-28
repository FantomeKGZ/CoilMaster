const fs = require('fs');

const read = path => fs.readFileSync(path, 'utf8');
const desktopRepairs = read('firmware/esp32/web/desktop/repairs.html');
const mobileRepairs = read('firmware/esp32/web/mobile/repairs.html');
const desktopMaterials = read('firmware/esp32/web/desktop/materials.html');
const mobileMaterials = read('firmware/esp32/web/mobile/materials.html');
const materialHeader = read('firmware/esp32/src/CM_MaterialLedger.h');
const materialWeb = read('firmware/esp32/src/CM_MaterialLedgerWeb.cpp');
const materialLedger = read('firmware/esp32/src/CM_MaterialLedger.cpp');
const materialSearch = read('firmware/esp32/src/CM_MaterialCatalogSearch.cpp');
const idempotencyHeader = read('firmware/esp32/src/CM_MaterialUsageIdempotency.h');
const idempotency = read('firmware/esp32/src/CM_MaterialUsageIdempotency.cpp');
const usageHistoryWeb = read('firmware/esp32/src/CM_MaterialUsageHistoryWeb.cpp');
const costingWeb = read('firmware/esp32/src/CM_RepairCostingWeb.cpp');
const runWire = read('firmware/esp32/src/CM_RunWireIssueCoordinator.cpp');

function must(source, token, label) {
  if (!source.includes(token)) throw new Error(`${label}: missing ${token}`);
}
function mustNot(source, token, label) {
  if (source.includes(token)) throw new Error(`${label}: forbidden ${token}`);
}

must(desktopRepairs, '/desktop/materials.html?repair_id=', 'desktop repair card material entry');
must(mobileRepairs, '/mobile/materials.html?repair_id=', 'mobile repair card material entry');

for (const [label, page] of [
  ['desktop materials', desktopMaterials],
  ['mobile materials', mobileMaterials]
]) {
  must(page, 'id="material"', `${label} warehouse selector`);
  must(page, "fetch('/api/materials?'+q", `${label} bounded authoritative catalog`);
  must(page, 'id="materialSearch"', `${label} material search input`);
  must(page, 'maxlength="48"', `${label} bounded search input`);
  must(page, "q.set('search',materialSearch)", `${label} server-side search query`);
  must(page, "materialSearch=value;load(0,true)", `${label} search cursor reset`);
  must(page, "materialSearch='';load(0,true)", `${label} clear search cursor reset`);
  must(page, 'stock_quantity_milli', `${label} stock preview`);
  must(page, 'price_per_unit_minor', `${label} price preview`);
  must(page, 'wire_type', `${label} wire type result metadata`);
  must(page, 'diameter_hundredths_mm', `${label} wire diameter result metadata`);
  must(page, "fetch('/api/materials/usage'", `${label} existing MaterialLedger writeoff route`);
  must(page, "fetch('/api/materials/usage?'+q", `${label} bounded usage history route`);
  must(page, "fetch('/api/repairs/costing?repair_id='", `${label} authoritative repair costing read`);
  must(page, 'id="usageHistory"', `${label} usage history surface`);
  must(page, 'id="materialCost"', `${label} material costing surface`);
  must(page, "new URLSearchParams({repair_id:repairId,limit:'20'})", `${label} bounded repair history query`);
  must(page, 'line_cost_minor', `${label} persisted line cost display`);
  must(page, 'material_cost_minor', `${label} costing material total display`);
  must(page, 'repairWritable', `${label} repair lifecycle gate`);
  must(page, 'Недостаточно материала', `${label} insufficient-stock precheck UX`);
  must(page, 'line_cost_source', `${label} persisted price snapshot response check`);
  must(page, 'pendingOperationId', `${label} retry operation state`);
  must(page, 'crypto.getRandomValues', `${label} strong operation id generation`);
  must(page, 'operation_id:pendingOperationId', `${label} operation id request`);
  must(page, 'expected_stock_quantity_milli:String(material.stock_quantity_milli)', `${label} stock snapshot request`);
  must(page, 'expected_price_per_unit_minor:String(material.price_per_unit_minor)', `${label} price snapshot request`);
  must(page, 'if(!pendingOperationId)pendingOperationId=newOperationId()', `${label} operation id reused across retry`);
  must(page, "$('quantity').oninput=resetOperationId", `${label} quantity change resets operation id`);
  must(page, "$('comment').oninput=resetOperationId", `${label} comment change resets operation id`);
  must(page, 'duplicate_replay===true', `${label} duplicate replay UX`);
  must(page, 'operation_id_conflict', `${label} operation collision UX`);
  must(page, "j.error==='insufficient_stock'", `${label} authoritative insufficient-stock response`);
  must(page, 'j.available_quantity_milli', `${label} current available stock display`);
  must(page, "j.error==='stale_material_preview'", `${label} stale preview response`);
  must(page, "j.error==='usage_not_committed'", `${label} mutation-time TOCTOU response`);
  must(page, 'проверьте новые данные и подтвердите списание ещё раз', `${label} stale preview manual reconfirm UX`);
  must(page, 'повторите вручную', `${label} final TOCTOU manual retry UX`);
  must(page, 'cleanUsageComment', `${label} system idempotency tag hidden from history`);
  must(page, 'Promise.all([load(0,true),loadUsageHistory(0,true),loadCosting()])', `${label} post-write authoritative refresh`);
  mustNot(page, 'name="material_id"', `${label} manual material id input`);
  mustNot(page, 'id="materialId"', `${label} manual material id field`);
  mustNot(page, '/api/warehouse/write-offs', `${label} legacy direct writeoff route`);
  mustNot(page, 'localStorage.setItem("materialCost"', `${label} duplicated costing persistence`);
}

must(materialHeader, 'static constexpr uint8_t MaxSearchLength = 48U;', 'bounded material search length');
must(materialHeader, 'const String& search,', 'search overload contract');
must(materialWeb, 'm_server.hasArg("search")', 'material API search input');
must(materialWeb, 'search.length() > MaterialLedger::MaxSearchLength', 'server search length gate');
must(materialWeb, 'm_ledger.appendMaterialsPageJson(response, search, cursor, limit, count,', 'server bounded search dispatch');
must(materialSearch, 'while (file.available() && count < limit)', 'bounded result accumulation');
must(materialSearch, 'file.readStringUntil', 'streaming NDJSON search');
must(materialSearch, 'nextCursor = hasMore ? pageEnd : 0UL;', 'cursor continuation');
must(materialSearch, 'FlatJsonObjectValidator::valid(line)', 'search structural validation');
must(materialSearch, 'status != "ACTIVE"', 'active-only search');
must(materialSearch, 'containsSearch(name, search)', 'name search');
must(materialSearch, 'containsSearch(comment, search)', 'comment search');
must(materialSearch, 'containsSearch(wireType, search)', 'wire type search');
must(materialSearch, 'diameterText', 'wire diameter search');
mustNot(materialSearch, 'std::vector', 'unbounded material result vector');
mustNot(materialSearch, 'readString()', 'whole-file material buffering');

must(idempotencyHeader, 'MinOperationIdLength = 16U', 'minimum operation id length');
must(idempotencyHeader, 'MaxOperationIdLength = 64U', 'maximum operation id length');
must(idempotency, 'while (file.available())', 'streaming usage replay scan');
must(idempotency, 'FlatJsonObjectValidator::valid(line)', 'usage replay structural validation');
must(idempotency, 'usageId <= previousUsageId', 'usage replay monotonic ID validation');
must(idempotency, 'if (replay.found)', 'duplicate operation-id fail-closed guard');
must(idempotency, 'replay.payloadMatches = storedRepairId == repairId &&', 'exact replay payload match');
must(idempotency, 'MU_TX=', 'generic usage transaction tag');
mustNot(idempotency, 'std::vector', 'unbounded replay storage');
mustNot(idempotency, 'readString()', 'whole-file replay buffering');

must(materialWeb, '!m_server.hasArg("operation_id")', 'generic operation id required');
must(materialWeb, 'parseUnsigned(m_server,"expected_stock_quantity_milli",0UL,0xFFFFFFFFUL,expectedStock)', 'expected stock preview input');
must(materialWeb, 'parseUnsigned(m_server,"expected_price_per_unit_minor",1UL,100000000UL,expectedPrice)', 'expected price preview input');
must(materialWeb, 'invalid_material_preview', 'invalid preview fail-closed response');
must(materialWeb, 'MaterialUsageIdempotency::lookup(SD,operationId,repairId,materialId,quantity,replay)', 'replay lookup before mutation');
must(materialWeb, 'operation_id_conflict', 'operation id collision response');
const replayBranchStart = materialWeb.indexOf('if(replay.found)');
const newWriteStart = materialWeb.indexOf('bool repairFound=false;', replayBranchStart);
if (replayBranchStart < 0 || newWriteStart < 0) throw new Error('duplicate replay branch boundaries missing');
const replayBranch = materialWeb.slice(replayBranchStart, newWriteStart);
must(replayBranch, 'duplicate_replay', 'duplicate replay response marker');
must(replayBranch, 'write_performed', 'duplicate replay no-write marker');
must(replayBranch, 'm_server.send(200', 'duplicate replay successful replay response');
must(replayBranch, 'replay.lineCostMinor', 'duplicate replay persisted line cost');
must(replayBranch, 'current.stockQuantityMilli', 'duplicate replay authoritative current stock');
must(materialWeb, 'MaterialUsageIdempotency::taggedComment(operationId,usageComment)', 'durable idempotency evidence');

must(materialWeb, 'm_ledger.loadActiveMaterialState(materialId,materialState,materialFound)', 'authoritative preview reread');
must(materialWeb, 'quantity>materialState.stockQuantityMilli', 'authoritative insufficient stock guard');
must(materialWeb, 'insufficient_stock', 'insufficient stock fail-closed response');
must(materialWeb, 'materialState.stockQuantityMilli!=expectedStock||materialState.pricePerUnitMinor!=expectedPrice', 'stale stock/price preview guard');
must(materialWeb, 'stale_material_preview', 'stale preview fail-closed response');
must(materialWeb, 'REFRESH_MATERIAL_AND_RETRY', 'mutation-time retry guidance');
const replayLookupPos = materialWeb.indexOf('MaterialUsageIdempotency::lookup(');
const staleGuardPos = materialWeb.indexOf('materialState.stockQuantityMilli!=expectedStock||materialState.pricePerUnitMinor!=expectedPrice');
const confirmPos = materialWeb.indexOf('m_ledger.confirmUsage(usage,result)');
if (replayLookupPos < 0 || staleGuardPos < 0 || confirmPos < 0 ||
    !(replayLookupPos < staleGuardPos && staleGuardPos < confirmPos)) {
  throw new Error('generic usage order must remain replay -> stale preview guard -> authoritative MaterialLedger mutation');
}
const beforeConfirm = materialWeb.slice(0, confirmPos);
if (beforeConfirm.includes('expectedPrice*') || beforeConfirm.includes('expectedPrice *') ||
    beforeConfirm.includes('expected_price_per_unit_minor*')) {
  throw new Error('client preview price must never become a cost authority');
}

must(materialWeb, 'm_ledger.confirmUsage(usage,result)', 'material route authoritative ledger mutation');
must(materialLedger, 'readMaterialState(usage.materialId, stockBefore, price, currency)', 'authoritative preflight reread');
must(materialLedger, 'rewriteQuantity(usage.materialId, usage.quantityMilli,', 'mutation-time stock rewrite/TOCTOU boundary');
must(usageHistoryWeb, 'appendUsageHistoryPageJson(', 'bounded server-side usage history');
must(costingWeb, 'summary.materialCostMinor', 'server authoritative material cost');

const runWireApplyStart = runWire.indexOf('bool RunWireIssueCoordinator::applyLedger(');
const runWirePhysicalStart = runWire.indexOf('bool RunWireIssueCoordinator::executePhysicalPhases(', runWireApplyStart);
if (runWireApplyStart < 0 || runWirePhysicalStart < 0 || runWirePhysicalStart <= runWireApplyStart) {
  throw new Error('RUN_WIRE dedicated ledger phase boundaries missing');
}
const runWireApply = runWire.slice(runWireApplyStart, runWirePhysicalStart);
must(runWireApply, 'm_ledger.confirmUsage(usage, result)', 'RUN_WIRE still uses dedicated coordinator ledger phase');
mustNot(runWire, 'MaterialUsageIdempotency', 'generic idempotency must not replace RUN_WIRE exact-run protection');
must(runWire, 'confirmedWriteOffForSourceRun', 'RUN_WIRE exact source-run duplicate protection remains');

console.log('Repair material card contracts OK: bounded search/history/costing, generic idempotency, stale-preview reconfirmation and final MaterialLedger TOCTOU remain protected without changing dedicated RUN_WIRE exact-run safety.');
