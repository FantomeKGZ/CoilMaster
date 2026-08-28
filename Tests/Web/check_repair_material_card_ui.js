const fs = require('fs');

const read = path => fs.readFileSync(path, 'utf8');
const desktopRepairs = read('firmware/esp32/web/desktop/repairs.html');
const mobileRepairs = read('firmware/esp32/web/mobile/repairs.html');
const desktopMaterials = read('firmware/esp32/web/desktop/materials.html');
const mobileMaterials = read('firmware/esp32/web/mobile/materials.html');
const materialWeb = read('firmware/esp32/src/CM_MaterialLedgerWeb.cpp');
const materialLedger = read('firmware/esp32/src/CM_MaterialLedger.cpp');

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
  must(page, 'stock_quantity_milli', `${label} stock preview`);
  must(page, 'price_per_unit_minor', `${label} price preview`);
  must(page, "fetch('/api/materials/usage'", `${label} existing MaterialLedger writeoff route`);
  must(page, 'repairWritable', `${label} repair lifecycle gate`);
  must(page, 'Недостаточно материала', `${label} insufficient-stock precheck UX`);
  must(page, 'line_cost_source', `${label} persisted price snapshot response check`);
  mustNot(page, 'name="material_id"', `${label} manual material id input`);
  mustNot(page, 'id="materialId"', `${label} manual material id field`);
  mustNot(page, '/api/warehouse/write-offs', `${label} legacy direct writeoff route`);
}

must(materialWeb, 'm_ledger.confirmUsage(usage,result)', 'material route authoritative ledger mutation');
must(materialLedger, 'readMaterialState(usage.materialId, stockBefore, price, currency)', 'authoritative preflight reread');
must(materialLedger, 'rewriteQuantity(usage.materialId, usage.quantityMilli,', 'mutation-time stock rewrite/TOCTOU boundary');

console.log('Repair material card entry contracts OK: desktop/mobile repair cards reuse the existing bounded MaterialLedger selector/writeoff flow without manual material IDs or a parallel ledger.');
