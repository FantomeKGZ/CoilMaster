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

must(publicSurface,
  'bool loadActiveMaterialState(uint32_t materialId,\n                                 MaterialItemState& state,\n                                 bool& found) const;',
  'public fail-closed state lookup');
must(publicSurface,
  'bool loadActiveMaterialCurrency(uint32_t materialId,\n                                    String& currency,\n                                    bool& found) const;',
  'public fail-closed currency lookup');
mustNot(header,
  'bool loadActiveMaterialState(uint32_t materialId,\n                                 MaterialItemState& state) const;',
  'ambiguous material-state wrapper');
mustNot(header,
  'bool loadActiveMaterialCurrency(uint32_t materialId, String& currency) const;',
  'ambiguous material-currency wrapper');
mustNot(source,
  'MaterialItemState& state) const\n{',
  'ambiguous material-state implementation');
mustNot(source,
  'String& currency) const\n{',
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
