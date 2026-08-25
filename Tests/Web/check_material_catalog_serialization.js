const fs = require('fs');

const source = fs.readFileSync('firmware/esp32/src/CM_MaterialLedger.cpp', 'utf8');

function must(text, needle, label) {
  if (!text.includes(needle)) throw new Error(`missing ${label}: ${needle}`);
}

must(source,
  'line += F("\\\",\\\"unit\\\":\\\""); line += unitText(material.unit);\n    line += F("\\\",\\\"stock_quantity_milli\\\":");',
  'closed unit JSON string before stock quantity');
must(source, 'FlatJsonObjectValidator::valid(line)', 'material file structural validation');
must(source, 'case MaterialUnit::Piece:', 'piece unit support');
must(source, 'case MaterialUnit::Gram:', 'gram unit support');
must(source, 'case MaterialUnit::Millilitre:', 'millilitre unit support');
must(source, 'case MaterialUnit::Metre:', 'metre unit support');
must(source, 'case MaterialUnit::SquareMetre:', 'square metre unit support');

if (source.includes('line += F(",\\\"stock_quantity_milli\\\":"); line += material.stockQuantityMilli;')) {
  throw new Error('unit JSON string is not closed before stock_quantity_milli');
}

console.log('Material catalog serialization contracts OK');
