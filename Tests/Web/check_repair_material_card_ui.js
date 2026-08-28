const fs = require('fs');

const read = path => fs.readFileSync(path, 'utf8');
const desktopRepairs = read('firmware/esp32/web/desktop/repairs.html');
const mobileRepairs = read('firmware/esp32/web/mobile/repairs.html');
const desktopMaterials = read('firmware/esp32/web/desktop/materials.html');
const mobileMaterials = read('firmware/esp32/web/mobile/materials.html');
const desktopWriteoff = read('firmware/esp32/web/desktop/writeoff.html');
const mobileWriteoff = read('firmware/esp32/web/mobile/writeoff.html');
const writeoffShared = read('firmware/esp32/web/shared/writeoff-spool-suggestion.js');
const correctionShared = read('firmware/esp32/web/shared/material-usage-corrections.js');
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
must(desktopMaterials, "'/desktop/writeoff.html?repair_id='+encodeURIComponent(repairId)", 'desktop materials exact repair RUN_WIRE entry');
must(mobileMaterials, "'/mobile/writeoff.html?repair_id='+encodeURIComponent(repairId)", 'mobile materials exact repair RUN_WIRE entry');

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
  must(page, "j.error==='usage_not_committed'", `${label} mutation-time ambiguous response`);
  must(page, 'проверьте новые данные и подтвердите списание ещё раз', `${label} stale preview manual reconfirm UX`);
  must(page, 'тот же идентификатор операции', `${label} ambiguous outcome same-operation guidance`);
  must(page, 'Не меняйте строку операции', `${label} transport error same-operation guidance`);
  must(page, 'id="runWireLink"', `${label} visible RUN_WIRE entry`);
  must(page, 'Провод RUN_WIRE', `${label} RUN_WIRE section`);
  must(page, 'RUN_COMPLETED сам по себе ничего не списывает', `${label} no automatic RUN_COMPLETED writeoff warning`);
  must(page, '/shared/material-usage-corrections.js', `${label} shared append-only correction controller`);
  mustNot(page, '/api/material-requests/warehouse', `${label} must not duplicate RUN_WIRE mutation endpoint`);
  mustNot(page, 'source_session_id:', `${label} must not construct RUN_WIRE source session`);
  mustNot(page, 'source_run_id:', `${label} must not construct RUN_WIRE source run`);
  mustNot(page, 'spool_id:String(', `${label} must not construct RUN_WIRE spool provenance`);
  const ambiguousStart = page.indexOf("if(r.status===409&&j.error==='usage_not_committed')");
  const ambiguousEnd = page.indexOf("if(r.status===404&&j.error==='repair_not_found')", ambiguousStart);
  if (ambiguousStart < 0 || ambiguousEnd < 0 || ambiguousEnd <= ambiguousStart) {
    throw new Error(`${label}: ambiguous usage retry branch boundaries missing`);
  }
  const ambiguousBranch = page.slice(ambiguousStart, ambiguousEnd);
  mustNot(ambiguousBranch, 'resetOperationId()', `${label} ambiguous outcome must preserve operation id`);
  must(ambiguousBranch, 'loadUsageHistory(0,true)', `${label} ambiguous outcome refreshes durable history`);
  must(page, 'cleanUsageComment', `${label} system idempotency tag hidden from history`);
  must(page, 'Promise.all([load(0,true),loadUsageHistory(0,true),loadCosting()])', `${label} post-write authoritative refresh`);
  mustNot(page, 'name="material_id"', `${label} manual material id input`);
  mustNot(page, 'id="materialId"', `${label} manual material id field`);
  mustNot(page, '/api/warehouse/write-offs', `${label} legacy direct writeoff route`);
  mustNot(page, 'localStorage.setItem("materialCost"', `${label} duplicated costing persistence`);
}

must(correctionShared, 'Коррекции списаний', 'correction UI section');
must(correctionShared, 'Исходное подтверждённое списание не редактируется и не удаляется', 'immutable source usage warning');
must(correctionShared, "new URLSearchParams({repair_id:repairId,limit:'20'})", 'bounded correction source history');
must(correctionShared, "fetch('/api/materials/usage?'+q", 'bounded source usage GET');
must(correctionShared, "fetch('/api/materials/usage/corrections?'+q", 'bounded correction history GET');
must(correctionShared, "fetch('/api/materials/usage/corrections',{method:'POST'", 'explicit correction POST');
must(correctionShared, 'pendingCorrectionOperationId', 'correction retry operation state');
must(correctionShared, 'crypto.getRandomValues', 'strong correction operation id');
must(correctionShared, 'operation_id:pendingCorrectionOperationId', 'correction operation id request');
must(correctionShared, 'if(!pendingCorrectionOperationId)pendingCorrectionOperationId=operationId()', 'same correction operation reused across retry');
must(correctionShared, "$('correctionUsage').onchange=()=>{resetCorrectionOperationId();renderSourceInfo()}", 'source change resets correction operation id');
must(correctionShared, "$('correctionQuantity').oninput=resetCorrectionOperationId", 'quantity change resets correction operation id');
must(correctionShared, "$('correctionComment').oninput=resetCorrectionOperationId", 'comment change resets correction operation id');
must(correctionShared, "String(x&&x.comment||'').indexOf('RWI_TX=')===0", 'RUN_WIRE source UI exclusion');
must(correctionShared, "j.error==='run_wire_correction_forbidden'", 'authoritative RUN_WIRE correction rejection');
must(correctionShared, "j.error==='usage_over_correction'", 'authoritative over-correction handling');
must(correctionShared, "j.source_cost_policy==='PERSISTED_USAGE_SNAPSHOT'", 'persisted source cost validation');
must(correctionShared, 'j.source_usage_immutable===true', 'source usage immutable validation');
must(correctionShared, "j.correction_history==='APPEND_ONLY'", 'append-only correction response validation');
must(correctionShared, 'material_correction_cost_minor', 'server correction cost total display');
must(correctionShared, 'material_correction_line_count', 'server correction count display');
must(correctionShared, 'Неоднозначный результат:', 'ambiguous correction result UX');
must(correctionShared, 'повтор отправит тот же correction operation_id', 'same-id ambiguous correction retry guidance');
const correctionCatch = correctionShared.indexOf("catch(err){result.className='bad';result.textContent='Неоднозначный результат:");
const correctionFinally = correctionShared.indexOf("finally{$('correctionSubmit').disabled=!writable}", correctionCatch);
if (correctionCatch < 0 || correctionFinally < 0) throw new Error('correction ambiguous catch boundaries missing');
mustNot(correctionShared.slice(correctionCatch, correctionFinally), 'resetCorrectionOperationId()', 'ambiguous correction outcome preserves operation id');
for (const forbidden of ['/api/material-requests/warehouse','source_session_id:','source_run_id:','spool_id:String(','method:\'DELETE\'','method:\'PUT\'']) {
  mustNot(correctionShared, forbidden, `correction UI must not contain ${forbidden}`);
}

must(desktopWriteoff, '/shared/writeoff-spool-suggestion.js', 'desktop dedicated RUN_WIRE workflow script');
must(mobileWriteoff, '/shared/writeoff-spool-suggestion.js', 'mobile dedicated RUN_WIRE workflow script');
must(writeoffShared, '/api/winding-history?repair_id=', 'RUN_WIRE completed-run evidence lookup');
must(writeoffShared, '/api/warehouse/write-offs?', 'RUN_WIRE exact coverage lookup');
must(writeoffShared, '/api/jobs/spool-selection?session_id=', 'RUN_WIRE immutable spool selection lookup');
must(writeoffShared, '/api/warehouse/spool-material-bridges?spool_id=', 'RUN_WIRE exact spool bridge lookup');
must(writeoffShared, '/api/material-requests?', 'RUN_WIRE repair material-request lookup');
must(writeoffShared, '/api/material-requests/status?', 'RUN_WIRE material-request status lookup');
must(writeoffShared, "confirmed:'true'", 'RUN_WIRE explicit operator confirmation');
must(writeoffShared, "source_kind:'RUN_WIRE'", 'RUN_WIRE dedicated source kind');
must(writeoffShared, 'source_session_id:sourceSessionId', 'RUN_WIRE exact source session request');
must(writeoffShared, 'source_run_id:sourceRunId', 'RUN_WIRE exact source run request');
must(writeoffShared, 'spool_id:String(activeSpool.spool_id)', 'RUN_WIRE immutable spool request');
must(writeoffShared, '/api/material-requests/warehouse', 'RUN_WIRE dedicated mutation endpoint');
mustNot(writeoffShared, '/api/materials/usage', 'RUN_WIRE UI must not use generic material usage endpoint');

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

console.log('Repair material card contracts OK: generic material safety, append-only generic corrections and dedicated exact-provenance RUN_WIRE remain separated and fail-closed.');
