const fs = require('fs');

const header = fs.readFileSync('firmware/esp32/src/CM_MaterialLedger.h', 'utf8');
const source = fs.readFileSync('firmware/esp32/src/CM_MaterialLedgerCurrency.cpp', 'utf8');
const repairRef = fs.readFileSync('firmware/esp32/src/CM_MaterialLedgerRepairReference.cpp', 'utf8');

function must(text, needle, label) {
  if (!text.includes(needle)) throw new Error(`missing ${label}: ${needle}`);
}
function mustNot(text, needle, label) {
  if (text.includes(needle)) throw new Error(`forbidden ${label}: ${needle}`);
}
function mustMatch(text, pattern, label) {
  if (!pattern.test(text)) throw new Error(`missing ${label}: ${pattern}`);
}
function mustNotMatch(text, pattern, label) {
  if (pattern.test(text)) throw new Error(`forbidden ${label}: ${pattern}`);
}

must(header, 'struct MaterialItemState', 'material state struct');
must(header, 'MaterialUnit unit;', 'state unit');
must(header, 'uint32_t stockQuantityMilli;', 'state stock');
must(header, 'uint32_t pricePerUnitMinor;', 'state price');

const classStart = header.indexOf('class MaterialLedger');
const privateStart = header.indexOf('\nprivate:', classStart);
if (classStart < 0 || privateStart < 0) throw new Error('MaterialLedger class/private boundary missing');
const publicSurface = header.slice(classStart, privateStart);

must(publicSurface,
  'bool repairExists(uint32_t repairId, bool& found) const;',
  'public fail-closed repair lookup');
mustNot(header,
  'bool repairExists(uint32_t repairId) const;',
  'ambiguous repair lookup wrapper');

mustMatch(publicSurface,
  /bool\s+loadActiveMaterialState\s*\(\s*uint32_t\s+materialId\s*,\s*MaterialItemState&\s+state\s*,\s*bool&\s+found\s*\)\s*const\s*;/,
  'public fail-closed state lookup');
mustMatch(publicSurface,
  /bool\s+loadActiveMaterialCurrency\s*\(\s*uint32_t\s+materialId\s*,\s*String&\s+currency\s*,\s*bool&\s+found\s*\)\s*const\s*;/,
  'public fail-closed currency lookup');
mustNotMatch(header,
  /bool\s+loadActiveMaterialState\s*\(\s*uint32_t\s+materialId\s*,\s*MaterialItemState&\s+state\s*\)\s*const\s*;/,
  'ambiguous material-state wrapper');
mustNotMatch(header,
  /bool\s+loadActiveMaterialCurrency\s*\(\s*uint32_t\s+materialId\s*,\s*String&\s+currency\s*\)\s*const\s*;/,
  'ambiguous material-currency wrapper');
mustNotMatch(source,
  /bool\s+MaterialLedger::loadActiveMaterialState\s*\(\s*uint32_t\s+materialId\s*,\s*MaterialItemState&\s+state\s*\)\s*const\s*\{/,
  'ambiguous material-state implementation');
mustNotMatch(source,
  /bool\s+MaterialLedger::loadActiveMaterialCurrency\s*\(\s*uint32_t\s+materialId\s*,\s*String&\s+currency\s*\)\s*const\s*\{/,
  'ambiguous material-currency implementation');

must(source, 'bool parseMaterialUnit(const String& value, MaterialUnit& unit)', 'canonical unit parser');
for (const unit of ['PIECE', 'GRAM', 'MILLILITRE', 'METRE', 'SQUARE_METRE']) {
  must(source, `value == "${unit}"`, `${unit} parser`);
}
must(source, '!FlatJsonObjectValidator::valid(line)', 'fail closed JSON validation');
must(source, 'currentId <= previousId', 'monotonic material IDs');
must(source, 'status != "ACTIVE"', 'active-only lookup');
must(source, 'state.stockQuantityMilli = stock;', 'stock snapshot');
must(source, 'state.pricePerUnitMinor = price;', 'price snapshot');
must(source, 'state.currency = storedCurrency;', 'currency snapshot');
must(source, 'found = false;', 'lookup initializes found');
must(source, 'loadActiveMaterialState(materialId, state, found)', 'currency lookup reuse');

must(repairRef,
  'bool MaterialLedger::repairExists(uint32_t repairId, bool& found) const',
  'fail-closed repair implementation');
must(repairRef, 'found = false;', 'repair lookup initializes found');
mustNot(repairRef,
  'bool MaterialLedger::repairExists(uint32_t repairId) const',
  'ambiguous repair lookup implementation');

console.log('Material catalog active-state lookup contracts OK: repair/state/currency reads preserve explicit found results and all ambiguous convenience wrappers remain removed.');
