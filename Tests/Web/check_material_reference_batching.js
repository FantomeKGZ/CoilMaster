const fs = require('fs');
const path = require('path');

const repoRoot = path.resolve(__dirname, '..', '..');
const auditPath = path.join(repoRoot, 'firmware', 'esp32', 'src', 'CM_MaterialPersistenceIntegrityAudit.cpp');
const audit = fs.readFileSync(auditPath, 'utf8');

function requireText(source, text, description) {
  if (!source.includes(text)) {
    throw new Error(`Missing ${description}: ${text}`);
  }
}

requireText(audit, 'constexpr uint8_t ReferenceBatchSize = 32U;', 'bounded material reference batch size');
requireText(audit, 'bool resolveExactReferences(fs::FS& storage,', 'batched exact reference resolver');
requireText(audit, 'if (reference.matches == 0xFFU || ++reference.matches > 1U)', 'duplicate reference fail-closed guard');
requireText(audit, 'if (references[index].matches != 1U) return false;', 'exactly-one reference requirement');

if (audit.includes('bool idExists(') || audit.includes('!idExists(')) {
  throw new Error('Per-record idExists scans must stay removed from material integrity audit');
}

requireText(audit, 'ExactIdReference materialReferences[ReferenceBatchSize];\n    ExactIdReference repairReferences[ReferenceBatchSize];', 'bounded usage reference arrays');
requireText(audit, '!resolveExactReferences(storage, MaterialsPath, "material_id",\n                                        materialReferences, batchCount) ||\n                !resolveExactReferences(storage, RepairsPath, "repair_id",\n                                        repairReferences, batchCount)', 'usage batched material and repair resolution');
requireText(audit, 'ExactIdReference materialReferences[ReferenceBatchSize];\n    uint8_t batchCount = 0U;', 'bounded adjustment material references');
requireText(audit, '!resolveExactReferences(storage, MaterialsPath, "material_id",\n                                        materialReferences, batchCount)', 'adjustment batched material resolution');

console.log('Material reference batching contracts OK: exact references remain fail-closed with bounded 32-row scans.');
