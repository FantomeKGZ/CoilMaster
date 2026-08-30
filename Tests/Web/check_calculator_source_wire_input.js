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

  if (!text.includes('id="sourceWires"')) fail('source wire/bundle input is missing');
  if (!text.includes("split(';')")) fail('semicolon-separated component parser is missing');
  if (!text.includes('raw.length>5')) fail('five-component UI limit is missing');
  if (!text.includes('[xх×]')) fail('strand-count xN syntax is missing');
  if (!text.includes('strands<1||strands>12')) fail('source strand-count 1..12 validation is missing');
  if (!text.includes('source_component_count')) fail('multi-component request count is missing');
  if (!text.includes("source_diameter_'")) fail('multi-component diameter args are missing');
  if (!text.includes("source_strands_'")) fail('multi-component strand args are missing');
  if (!text.includes("q.set('source_strands_'+n,String(c.strands))")) fail('entered source strand counts are not sent to API');
  if (text.includes("q.set('source_strands_'+n,'1')")) fail('source strand count is still hard-coded to one');
  if (!text.includes('0,80x3')) fail('operator example for repeated source strands is missing');

  if (!text.includes('id="standardResults"')) fail('standard recommendation block is missing');
  if (!text.includes('data.standard_recommendations')) fail('standard recommendation API result is not rendered');
  if (!text.includes('data.standard_catalogue_diameter_count')) fail('standard catalogue count is not surfaced');
  if (!text.includes('Можно закупить')) fail('non-stock standard option label is missing');
}

const staticSitePath = 'firmware/esp32/src/CM_StaticSiteServer.cpp';
const staticSite = fs.readFileSync(staticSitePath, 'utf8');
const legacyHelperPath = 'firmware/esp32/web/shared/calculator-multisource.js';
if (staticSite.includes('calculator-multisource.js')) {
  console.error(`${staticSitePath}: obsolete calculator helper injection must not return`);
  process.exitCode = 1;
}
if (fs.existsSync(legacyHelperPath)) {
  console.error(`${legacyHelperPath}: obsolete calculator helper must remain removed`);
  process.exitCode = 1;
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
  'parseUnsignedArg(m_server, strandsName.c_str(), 1UL, 12UL, strands)',
]) {
  if (!api.includes(required)) {
    console.error(`${apiPath}: missing calculator contract: ${required}`);
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
  console.log('Calculator contracts OK: source diameter x strand-count input, API propagation, warehouse recommendations, and read-only IEC standard alternatives');
}
