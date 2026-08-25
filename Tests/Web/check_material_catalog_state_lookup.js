const fs = require('fs');

const header = fs.readFileSync('firmware/esp32/src/CM_MaterialLedger.h', 'utf8');
const source = fs.readFileSync('firmware/esp32/src/CM_MaterialLedgerCurrency.cpp', 'utf8');

function must(text, needle, label) {
  if (!text.includes(needle)) throw new Error(`missing ${label}: ${needle}`);
}

must(header, 'struct MaterialItemState', 'material state struct');
must(header, 'MaterialUnit unit;', 'state unit');
must(header, 'uint32_t stockQuantityMilli;', 'state stock');
must(header, 'uint32_t pricePerUnitMinor;', 'state price');
must(header, 'bool loadActiveMaterialState(uint32_t materialId,', 'state lookup API');

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
must(source, 'loadActiveMaterialState(materialId, state, found)', 'currency lookup reuse');

console.log('Material catalog active-state lookup contracts OK');
