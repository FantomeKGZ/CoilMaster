const fs = require('fs');

const audit = fs.readFileSync('firmware/esp32/src/CM_CrmPersistenceIntegrityAudit.cpp', 'utf8');
const backup = fs.readFileSync('firmware/esp32/src/CM_BackupExportWeb.cpp', 'utf8');

if (!audit.includes('/data/workshop/material-request-status.ndjson')) {
  throw new Error('CRM integrity audit does not cover material-request-status.ndjson');
}
if (!backup.includes('/data/workshop/material-request-status.ndjson')) {
  throw new Error('backup export does not include material-request-status.ndjson');
}

console.log('Material Request status backup/integrity coverage OK');
