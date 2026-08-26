const fs = require('fs');

const header = fs.readFileSync('firmware/esp32/src/CM_MaterialRequestUnitAdapter.h', 'utf8');
const source = fs.readFileSync('firmware/esp32/src/CM_MaterialRequestUnitAdapter.cpp', 'utf8');
const movement = fs.readFileSync('firmware/esp32/src/CM_MaterialRequestMovementStore.cpp', 'utf8');

function must(text, needle, label) {
  if (!text.includes(needle)) throw new Error(`missing ${label}: ${needle}`);
}

must(header, 'uint32_t ledgerQuantityMilli;', 'ledger quantity output');
must(header, 'uint64_t requestUnitCostMinor;', 'request unit cost output');
must(header, 'uint64_t costAmountMinor;', 'cost output');

must(source, 'requestUnit == "KG"', 'KG mapping');
must(source, 'ledgerUnit = MaterialUnit::Gram;', 'KG to GRAM');
must(source, 'requestUnit == "L"', 'L mapping');
must(source, 'ledgerUnit = MaterialUnit::Millilitre;', 'L to MILLILITRE');
must(source, 'requestUnit == "PCS"', 'PCS mapping');
must(source, 'ledgerUnit = MaterialUnit::Piece;', 'PCS to PIECE');
must(source, 'requestUnit == "M"', 'M mapping');
must(source, 'ledgerUnit = MaterialUnit::Metre;', 'M to METRE');
must(source, 'requestUnit == "M2"', 'M2 mapping');
must(source, 'ledgerUnit = MaterialUnit::SquareMetre;', 'M2 to SQUARE_METRE');
must(source, 'scale = 1000UL;', 'mass/volume scale');
must(source, 'requestQuantityMilli > 0xFFFFFFFFUL / scale', 'quantity overflow guard');
must(source, '0xFFFFFFFFFFFFFFFFULL - 500ULL', 'cost overflow guard');
must(source, '+\n         500ULL) /\n        1000ULL;', 'nearest-minor cost rounding');

must(movement, 'unit == "M2"', 'movement M2 support');
must(movement, 'movement.unit == "KG"', 'RUN_WIRE stays KG-only');

require('./check_spool_material_bridge_store.js');
console.log('Material Request unit adapter contracts OK');
