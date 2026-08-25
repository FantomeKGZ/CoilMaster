const fs = require('fs');

const backup = fs.readFileSync('firmware/esp32/src/CM_BackupExportWeb.cpp', 'utf8');
const header = fs.readFileSync('firmware/esp32/src/CM_CrmPersistenceIntegrityAudit.h', 'utf8');
const source = fs.readFileSync('firmware/esp32/src/CM_CrmPersistenceIntegrityAudit.cpp', 'utf8');

function must(text, needle, label) {
  if (!text.includes(needle)) throw new Error(`missing ${label}: ${needle}`);
}

must(backup, '#include "CM_CrmPersistenceIntegrityAudit.h"', 'CRM audit wiring');
must(backup, '"motor-winding-versions", "/data/workshop/motor-winding-versions.ndjson"', 'winding versions export');
must(backup, '"repair-as-received", "/data/workshop/repair-as-received.ndjson"', 'AS_RECEIVED export');
must(backup, '"material-requests", "/data/workshop/material-requests.ndjson"', 'material requests export');
must(backup, '"material-request-movements", "/data/workshop/material-request-movements.ndjson"', 'request movements export');
must(backup, '"/data/workshop/repair-intake.pending.json", "repair_intake_pending"', 'pending marker backup block');
must(backup, '"/data/workshop/repair-intake.pending.tmp", "repair_intake_temp_present"', 'temp marker backup block');
must(backup, 'CrmPersistenceIntegrityAudit::check(storage, crmMetrics)', 'CRM snapshot audit');
must(backup, 'return "crm_persistence_unstable_or_invalid";', 'fail-closed CRM backup reason');

must(header, 'windingVersionRecordCount', 'winding metrics');
must(header, 'asReceivedRecordCount', 'snapshot metrics');
must(header, 'materialRequestRecordCount', 'request metrics');
must(header, 'materialRequestMovementRecordCount', 'movement metrics');

must(source, '/data/workshop/motor-winding-versions.ndjson', 'winding source audit');
must(source, '/data/workshop/repair-as-received.ndjson', 'snapshot source audit');
must(source, '/data/workshop/material-requests.ndjson', 'request source audit');
must(source, '/data/workshop/material-request-movements.ndjson', 'movement source audit');
must(source, 'ReferenceBatchSize = 24U', 'bounded reference batching');
must(source, 'resolveReferences(storage, MotorsPath', 'motor cross references');
must(source, 'resolveReferences(storage, RepairsPath', 'repair cross references');
must(source, 'resolveReferences(storage, ClientsPath', 'client cross references');
must(source, 'resolveReferences(storage, MaterialRequestsPath', 'movement request cross references');
must(source, 'WindingProgramParser::valid', 'winding program validation');

if (source.includes('m_storage.remove(') || source.includes('storage.remove(')) {
  throw new Error('CRM integrity audit must be read-only');
}

console.log('CRM backup/integrity contracts OK');
