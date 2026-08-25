const fs = require('fs');

const source = fs.readFileSync(
  'firmware/esp32/src/CM_RepairPricingIntegrityAudit.cpp',
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
  'Repair pricing audit must keep bounded reference batch size 32.'
);

requireContract(
  source.includes('bool resolveRepairReferences('),
  'Repair pricing audit must resolve repair references in batches.'
);

requireContract(
  !source.includes('bool repairExists(') && !source.includes('repairExists(storage'),
  'Repair pricing audit must not reopen repairs.ndjson once per pricing row.'
);

requireContract(
  source.includes('reference.matches == 0xFFU || ++reference.matches > 1U'),
  'Repair pricing batch resolver must fail closed on duplicate repair IDs.'
);

requireContract(
  source.includes('if (references[index].matches != 1U) return false;'),
  'Repair pricing batch resolver must require exactly one repair match.'
);

requireContract(
  /if\s*\(batchCount\s*==\s*ReferenceBatchSize\)[\s\S]*?resolveRepairReferences\(storage,\s*references,\s*batchCount\)/.test(source),
  'Repair pricing audit must resolve each full bounded batch.'
);

requireContract(
  /if\s*\(batchCount\s*>\s*0U\s*&&[\s\S]*?!resolveRepairReferences\(storage,\s*references,\s*batchCount\)\)/.test(source),
  'Repair pricing audit must resolve the final partial batch.'
);

console.log('Repair pricing reference batching contracts OK');
