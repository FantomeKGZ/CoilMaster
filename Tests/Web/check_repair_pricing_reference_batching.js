const fs = require('fs');

const source = fs.readFileSync(
  'firmware/esp32/src/CM_BackupBusinessDataIntegrityAudit.cpp',
  'utf8'
);

function requireContract(condition, message) {
  if (!condition) {
    console.error(message);
    process.exit(1);
  }
}

requireContract(
  /constexpr\s+uint8_t\s+ReferenceBatchSize\s*=\s*32U\s*;/.test(source),
  'Business-data pricing audit must keep bounded reference batch size 32.'
);

requireContract(
  source.includes('bool validatePricingBatch(') &&
  source.includes('resolveReferences(storage, RepairsPath, "repair_id", references, count)'),
  'Business-data pricing audit must resolve repair references in bounded batches.'
);

requireContract(
  !source.includes('bool repairExists(') && !source.includes('repairExists(storage'),
  'Business-data pricing audit must not reopen repairs.ndjson once per pricing row.'
);

requireContract(
  source.includes('bool validatePricing(fs::FS& storage, uint32_t& recordCount)'),
  'Authoritative business-data audit must validate pricing.ndjson.'
);

requireContract(
  source.includes('!FlatJsonObjectValidator::valid(line)') &&
  source.includes('!findUnsigned(line, "repair_id", repairId)') &&
  source.includes('!findUnsigned64(line, "labour_cost_minor", labour)') &&
  source.includes('!findUnsigned64(line, "client_price_minor", client)') &&
  source.includes('currency.length() != 3U') &&
  source.includes('timestamp.length() < 10U'),
  'Authoritative pricing audit must retain fail-closed pricing schema validation.'
);

requireContract(
  /if\s*\(batchCount\s*==\s*ReferenceBatchSize\)[\s\S]*?!validatePricingBatch\(storage,\s*references,\s*batchCount\)/.test(source),
  'Business-data pricing audit must resolve each full bounded batch.'
);

requireContract(
  /if\s*\(batchCount\s*>\s*0U\s*&&\s*!validatePricingBatch\(storage,\s*references,\s*batchCount\)\)/.test(source),
  'Business-data pricing audit must resolve the final partial batch.'
);

requireContract(
  source.includes('validatePricing(storage, metrics.pricingRecordCount)'),
  'Pricing validation must remain part of the authoritative business-data backup audit.'
);

console.log('Repair pricing reference batching contracts OK: authoritative BackupBusinessDataIntegrityAudit owns bounded pricing validation and exact repair references.');
