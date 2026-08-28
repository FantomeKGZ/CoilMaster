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
const usageHistoryWeb = read('firmware/esp32/src/CM_MaterialUsageHistoryWeb.cpp');
const costingWeb = read('firmware/esp32/src/CM_RepairCostingWeb.cpp');

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

must(materialWeb, 'm_ledger.confirmUsage(usage,result)', 'material route authoritative ledger mutation');
must(materialLedger, 'readMaterialState(usage.materialId, stockBefore, price, currency)', 'authoritative preflight reread');
must(materialLedger, 'rewriteQuantity(usage.materialId, usage.quantityMilli,', 'mutation-time stock rewrite/TOCTOU boundary');
must(usageHistoryWeb, 'appendUsageHistoryPageJson(', 'bounded server-side usage history');
must(costingWeb, 'summary.materialCostMinor', 'server authoritative material cost');

console.log('Repair material card contracts OK: desktop/mobile use bounded server-side warehouse search plus authoritative MaterialLedger history/costing while preserving the existing mutation and TOCTOU boundaries.');
