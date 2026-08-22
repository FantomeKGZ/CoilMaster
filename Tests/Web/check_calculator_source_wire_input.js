const fs = require('fs');

const files = [
  'firmware/esp32/web/desktop/calculator.html',
  'firmware/esp32/web/mobile/calculator.html',
];

for (const file of files) {
  const text = fs.readFileSync(file, 'utf8');
  const fail = (message) => {
    console.error(`${file}: ${message}`);
    process.exitCode = 1;
  };

  if (!text.includes('id="sourceWires"')) fail('single source wire input is missing');
  if (!text.includes("split(';')")) fail('semicolon-separated wire parser is missing');
  if (!text.includes('raw.length>5')) fail('five-wire UI limit is missing');
  if (!text.includes('source_component_count')) fail('multi-component request count is missing');
  if (!text.includes("source_diameter_'")) fail('multi-component diameter args are missing');
  if (!text.includes("source_strands_'")) fail('multi-component strand args are missing');
  if (!text.includes("q.set('source_strands_'+n,'1')")) fail('each entered diameter must represent one source wire');
  if (text.includes('id="diameter"') || text.includes('id="strands"')) {
    fail('legacy diameter/strand input pair must not return');
  }

  if (!text.includes('id="standardResults"')) fail('standard recommendation block is missing');
  if (!text.includes('data.standard_recommendations')) fail('standard recommendation API result is not rendered');
  if (!text.includes('data.standard_catalogue_diameter_count')) fail('standard catalogue count is not surfaced');
  if (!text.includes('Можно закупить')) fail('non-stock standard option label is missing');
}

const apiPath = 'firmware/esp32/src/CM_ConductorCalculatorWeb.cpp';
const api = fs.readFileSync(apiPath, 'utf8');
const cataloguePath = 'firmware/esp32/src/CM_StandardWireCatalogue.cpp';
const catalogue = fs.readFileSync(cataloguePath, 'utf8');

for (const required of [
  '#include "CM_StandardWireCatalogue.h"',
  'StandardWireCatalogue::load(',
  'standard_catalogue_basis',
  'standard_catalogue_diameter_count',
  'standard_recommendations',
  'warehouse_available',
  'diameter_storage_precision_mm',
]) {
  if (!api.includes(required)) {
    console.error(`${apiPath}: missing standard calculator contract: ${required}`);
    process.exitCode = 1;
  }
}

if (api.includes('wire_catalogue_empty_for_material')) {
  console.error(`${apiPath}: empty warehouse catalogue must not disable standard recommendations`);
  process.exitCode = 1;
}

for (const required of [
  'CopperR20Hundredths',
  'AluminiumR20Hundredths',
  'IEC_60317_R20_PROJECT_0_01_MM',
  'candidates[index].availableGrams = 1UL;',
]) {
  if (!catalogue.includes(required)) {
    console.error(`${cataloguePath}: missing read-only standard catalogue contract: ${required}`);
    process.exitCode = 1;
  }
}

for (const forbidden of ['SD.', 'FILE_WRITE', 'FILE_APPEND', 'addSpool(', 'setWarehouse']) {
  if (catalogue.includes(forbidden)) {
    console.error(`${cataloguePath}: standard reference catalogue must remain read-only: ${forbidden}`);
    process.exitCode = 1;
  }
}

if (!process.exitCode) {
  console.log('Calculator contracts OK: semicolon source wires, warehouse recommendations, and read-only IEC standard alternatives');
}
