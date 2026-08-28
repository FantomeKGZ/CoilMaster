const fs=require('fs');
const read=p=>fs.readFileSync(p,'utf8');
const must=(t,n,l)=>{if(!t.includes(n))throw new Error(`missing ${l}: ${n}`)};
const mustNot=(t,n,l)=>{if(t.includes(n))throw new Error(`forbidden ${l}: ${n}`)};

const ledgerH=read('firmware/esp32/src/CM_MaterialLedger.h');
const correction=read('firmware/esp32/src/CM_MaterialUsageCorrection.cpp');
const correctionHistory=read('firmware/esp32/src/CM_MaterialUsageCorrectionHistory.cpp');
const correctionAudit=read('firmware/esp32/src/CM_MaterialUsageCorrectionIntegrityAudit.cpp');
const correctionWeb=read('firmware/esp32/src/CM_MaterialUsageCorrectionWeb.cpp');
const materialAudit=read('firmware/esp32/src/CM_MaterialPersistenceIntegrityAudit.cpp');
const costing=read('firmware/esp32/src/CM_RepairCosting.cpp');
const costingWeb=read('firmware/esp32/src/CM_RepairCostingWeb.cpp');
const warehouseWeb=read('firmware/esp32/src/CM_WarehouseWeb.cpp');

must(ledgerH,'bool correctUsage(const MaterialUsageCorrection& correction, MaterialUsageCorrectionResult& result);','correction mutation API');
must(ledgerH,'appendUsageCorrectionHistoryPageJson','bounded correction history reader');
for(const field of ['correctionSourceUsageId','correctionRepairId','correctionQuantityMilli','correctionLineCostMinor','correctionOperationId'])must(ledgerH,field,`correction provenance ${field}`);

must(correction,'sourceComment.indexOf(F("RWI_TX=")) == 0','generic correction rejects RUN_WIRE evidence');
must(correction,'MaterialUsageCorrectionStatus::RunWireForbidden','RUN_WIRE domain status');
must(correction,'MaterialUsageCorrectionStatus::OverCorrection','over-correction status');
must(correction,'MaterialUsageCorrectionStatus::OperationIdConflict','operation id conflict');
must(correction,'MaterialUsageCorrectionStatus::DuplicateReplay','idempotent replay');
must(correction,'adjustment.correctionSourceUsageId = correction.sourceUsageId;','exact source usage provenance');
must(correction,'adjustment.correctionOperationId = correction.operationId;','durable correction operation id');
must(correction,'(static_cast<uint64_t>(correction.quantityMilli) * sourcePrice + 500ULL) / 1000ULL','persisted source price correction cost');
mustNot(correction,'remove(UsagePath)','source usage history is immutable');

must(correctionAudit,'constexpr uint8_t BatchSize = 16U;','bounded correction integrity batch');
must(correctionAudit,'ref.cumulativeQuantity > quantity','cumulative correction guard');
must(correctionAudit,'comment.indexOf(F("RWI_TX=")) == 0','integrity rejects RUN_WIRE source');
must(correctionAudit,'ref.lineCostMinor != (correctionProduct + 500ULL) / 1000ULL','integrity validates persisted-cost formula');
must(materialAudit,'MaterialUsageCorrectionIntegrityAudit::check(storage)','material backup/integrity correction coverage');

must(correctionHistory,'limit > MaxListPageSize','history bounded by ledger page size');
must(correctionHistory,'adjustmentId <= cursor','history cursor semantics');
must(correctionHistory,'MaterialUsageCorrectionIntegrityAudit::check(m_storage)','history integrity preflight');

must(correctionWeb,'/api/materials/usage/corrections','dedicated correction endpoint');
must(correctionWeb,'HTTP_GET','bounded correction history GET');
must(correctionWeb,'HTTP_POST','explicit correction POST');
must(correctionWeb,'RepairLifecycle::isOpen','OPEN repair mutation gate');
must(correctionWeb,'run_wire_correction_forbidden','actionable RUN_WIRE rejection');
must(correctionWeb,'source_cost_policy\\\":\\\"PERSISTED_USAGE_SNAPSHOT','persisted source cost response');
must(correctionWeb,'source_usage_immutable\\\":true','immutable source usage response');
must(correctionWeb,'correction_history\\\":\\\"APPEND_ONLY','append-only response contract');

must(warehouseWeb,'static MaterialUsageCorrectionWeb materialUsageCorrectionWeb(m_server, materialLedger);','same authoritative MaterialLedger instance');
must(warehouseWeb,'materialUsageCorrectionWeb.begin();','correction endpoint production registration');

must(costing,'MaterialUsageCorrectionIntegrityAudit::check(m_storage)','costing correction integrity gate');
must(costing,'summary.materialCostMinor -= summary.materialCorrectionCostMinor;','net generic material costing');
must(costing,'summary.materialCorrectionCostMinor > summary.materialCostMinor','costing underflow fail closed');
must(costingWeb,'material_correction_cost_minor','correction cost API');
must(costingWeb,'material_correction_line_count','correction count API');
must(costingWeb,'CONFIRMED_USAGE_MINUS_APPEND_ONLY_CORRECTIONS','net material costing source');

console.log('Material usage correction contracts OK: append-only exact usage provenance, RUN_WIRE isolation, idempotent replay, bounded history, integrity coverage and net costing remain fail-closed.');
