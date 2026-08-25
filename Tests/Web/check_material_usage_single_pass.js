const fs = require('fs');

function fail(message) {
  console.error(`FAIL: ${message}`);
  process.exit(1);
}

function bodyBetween(source, startMarker, endMarker) {
  const start = source.indexOf(startMarker);
  if (start < 0) fail(`missing ${startMarker}`);
  const end = source.indexOf(endMarker, start + startMarker.length);
  if (end < 0) fail(`missing end marker ${endMarker}`);
  return source.slice(start, end);
}

const ledger = fs.readFileSync('firmware/esp32/src/CM_MaterialLedger.cpp', 'utf8');
const adjustment = fs.readFileSync('firmware/esp32/src/CM_MaterialAdjustment.cpp', 'utf8');

const confirm = bodyBetween(
  ledger,
  'bool MaterialLedger::confirmUsage(',
  'bool MaterialLedger::ensureDirectories()'
);

if (!confirm.includes('readMaterialState(usage.materialId, stockBefore, price, currency)')) {
  fail('confirmUsage must use one authoritative material-state preflight scan');
}
if (confirm.includes('readStockQuantity(')) {
  fail('confirmUsage must not perform a separate stock-only preflight scan');
}
if (confirm.includes('m_storage.open(MaterialsPath, FILE_READ)')) {
  fail('confirmUsage must not reopen the material catalog for pricing preflight');
}
if (!confirm.includes('currency != "KGS"')) {
  fail('confirmUsage must preserve KGS currency gate');
}
if (!confirm.includes('rewriteQuantity(usage.materialId, usage.quantityMilli,')) {
  fail('confirmUsage must retain the mutation/transaction rewrite pass');
}

const state = bodyBetween(
  adjustment,
  'bool MaterialLedger::readMaterialState(',
  'bool MaterialLedger::appendAdjustmentLine('
);

for (const required of [
  'FlatJsonObjectValidator::valid(line)',
  'currentId <= previousId',
  'findUnsigned(line, "stock_quantity_milli", stock)',
  'findUnsigned(line, "price_per_unit_minor", price)',
  'price == 0UL',
  'findString(line, "currency", lineCurrency)',
  'lineCurrency.length() != 3U',
  'findString(line, "status", status)',
  'found || status != "ACTIVE"'
]) {
  if (!state.includes(required)) fail(`readMaterialState lost fail-closed contract: ${required}`);
}

console.log('PASS: material usage preflight is single-pass and fail-closed');
