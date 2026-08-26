const fs = require('fs');

const backup = fs.readFileSync('firmware/esp32/src/CM_BackupExportWeb.cpp', 'utf8');
const header = fs.readFileSync('firmware/esp32/src/CM_CrmPersistenceIntegrityAudit.h', 'utf8');
const source = fs.readFileSync('firmware/esp32/src/CM_CrmPersistenceIntegrityAudit.cpp', 'utf8');
const deliveryStoreHeader = fs.readFileSync('firmware/esp32/src/CM_RepairDeliveryStore.h', 'utf8');
const deliveryStore = fs.readFileSync('firmware/esp32/src/CM_RepairDeliveryStore.cpp', 'utf8');
const deliveryWeb = fs.readFileSync('firmware/esp32/src/CM_RepairDeliveryWeb.cpp', 'utf8');
const deliveryAudit = fs.readFileSync('firmware/esp32/src/CM_RepairDeliveryIntegrityAudit.cpp', 'utf8');
const repairWeb = fs.readFileSync('firmware/esp32/src/CM_RepairRegistryWeb.cpp', 'utf8');

function must(text, needle, label) {
  if (!text.includes(needle)) throw new Error(`missing ${label}: ${needle}`);
}

must(backup, '#include "CM_CrmPersistenceIntegrityAudit.h"', 'CRM audit wiring');
must(backup, '"motor-winding-versions", "/data/workshop/motor-winding-versions.ndjson"', 'winding versions export');
must(backup, '"repair-as-received", "/data/workshop/repair-as-received.ndjson"', 'AS_RECEIVED export');
must(backup, '"material-requests", "/data/workshop/material-requests.ndjson"', 'material requests export');
must(backup, '"material-request-movements", "/data/workshop/material-request-movements.ndjson"', 'request movements export');
must(backup, '"repair-deliveries", "/data/workshop/repair-deliveries.ndjson"', 'repair deliveries export');
must(backup, '"/data/workshop/repair-intake.pending.json", "repair_intake_pending"', 'pending marker backup block');
must(backup, '"/data/workshop/repair-intake.pending.tmp", "repair_intake_temp_present"', 'temp marker backup block');
must(backup, 'CrmPersistenceIntegrityAudit::check(storage, crmMetrics)', 'CRM snapshot audit');
must(backup, 'RepairDeliveryIntegrityAudit::check(storage, repairDeliveryRecordCount)', 'delivery snapshot audit');
must(backup, 'return "crm_persistence_unstable_or_invalid";', 'fail-closed CRM backup reason');
must(backup, 'return "repair_delivery_unstable_or_invalid";', 'fail-closed delivery backup reason');

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

must(deliveryStoreHeader, '/data/workshop/repair-deliveries.ndjson', 'delivery journal path');
must(deliveryStore, 'FILE_APPEND', 'append-only delivery evidence');
must(deliveryStore, 'resolveByRepair(delivery.repairId, existing, found)', 'single delivery per repair gate');
must(deliveryWeb, '"/api/repairs/delivery"', 'delivery API route');
must(deliveryWeb, 'explicit_confirmation_required', 'delivery explicit confirmation');
must(deliveryWeb, 'm_repairs.loadRepairIdentity', 'server-side delivery identity');
must(deliveryWeb, 'm_repairs.repairIsOpen', 'delivery repair lifecycle lookup');
must(deliveryWeb, 'repair_must_be_closed_before_delivery', 'CLOSED repair delivery gate');
must(deliveryWeb, 'balance_gate_applied\\\":false', 'delivery allowed independently of balance');
must(repairWeb, 'static RepairDeliveryStore deliveryStore(SD);', 'delivery store production bootstrap');
must(repairWeb, 'if (deliveryStore.begin()) deliveryWeb.begin();', 'delivery API production registration');

must(deliveryAudit, 'repairIdentityMatches', 'delivery exact repair identity audit');
must(deliveryAudit, 'deliveryUniqueForRepair', 'delivery uniqueness audit');
must(deliveryAudit, 'RepairLifecycle::isOpen', 'delivery CLOSED repair audit');
must(deliveryAudit, 'repairOpen', 'delivery open-state fail-closed audit');

if (source.includes('m_storage.remove(') || source.includes('storage.remove(') ||
    deliveryAudit.includes('storage.remove(')) {
  throw new Error('CRM/delivery integrity audits must be read-only');
}
if (deliveryWeb.includes('payment') || deliveryWeb.includes('balanceMinor')) {
  throw new Error('delivery persistence must not require cash balance/payment state');
}

console.log('CRM backup/integrity + repair delivery contracts OK');
