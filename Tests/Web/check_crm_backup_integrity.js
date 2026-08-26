const fs = require('fs');

const backup = fs.readFileSync('firmware/esp32/src/CM_BackupExportWeb.cpp', 'utf8');
const header = fs.readFileSync('firmware/esp32/src/CM_CrmPersistenceIntegrityAudit.h', 'utf8');
const source = fs.readFileSync('firmware/esp32/src/CM_CrmPersistenceIntegrityAudit.cpp', 'utf8');
const deliveryStoreHeader = fs.readFileSync('firmware/esp32/src/CM_RepairDeliveryStore.h', 'utf8');
const deliveryStore = fs.readFileSync('firmware/esp32/src/CM_RepairDeliveryStore.cpp', 'utf8');
const deliveryWeb = fs.readFileSync('firmware/esp32/src/CM_RepairDeliveryWeb.cpp', 'utf8');
const deliveryAudit = fs.readFileSync('firmware/esp32/src/CM_RepairDeliveryIntegrityAudit.cpp', 'utf8');
const repairWeb = fs.readFileSync('firmware/esp32/src/CM_RepairRegistryWeb.cpp', 'utf8');
const cashStoreHeader = fs.readFileSync('firmware/esp32/src/CM_CashPaymentStore.h', 'utf8');
const cashStore = fs.readFileSync('firmware/esp32/src/CM_CashPaymentStore.cpp', 'utf8');
const cashWeb = fs.readFileSync('firmware/esp32/src/CM_CashPaymentWeb.cpp', 'utf8');
const cashAudit = fs.readFileSync('firmware/esp32/src/CM_CashPaymentIntegrityAudit.cpp', 'utf8');
const materialWeb = fs.readFileSync('firmware/esp32/src/CM_MaterialLedgerWeb.cpp', 'utf8');

function must(text, needle, label) {
  if (!text.includes(needle)) throw new Error(`missing ${label}: ${needle}`);
}

must(backup, '#include "CM_CrmPersistenceIntegrityAudit.h"', 'CRM audit wiring');
must(backup, '"motor-winding-versions", "/data/workshop/motor-winding-versions.ndjson"', 'winding versions export');
must(backup, '"repair-as-received", "/data/workshop/repair-as-received.ndjson"', 'AS_RECEIVED export');
must(backup, '"material-requests", "/data/workshop/material-requests.ndjson"', 'material requests export');
must(backup, '"material-request-movements", "/data/workshop/material-request-movements.ndjson"', 'request movements export');
must(backup, '"repair-deliveries", "/data/workshop/repair-deliveries.ndjson"', 'repair deliveries export');
must(backup, '"repair-payments", "/data/workshop/repair-payments.ndjson"', 'repair payments export');
must(backup, '"/data/workshop/repair-intake.pending.json", "repair_intake_pending"', 'pending marker backup block');
must(backup, '"/data/workshop/repair-intake.pending.tmp", "repair_intake_temp_present"', 'temp marker backup block');
must(backup, 'CrmPersistenceIntegrityAudit::check(storage, crmMetrics)', 'CRM snapshot audit');
must(backup, 'RepairDeliveryIntegrityAudit::check(storage, repairDeliveryRecordCount)', 'delivery snapshot audit');
must(backup, 'CashPaymentIntegrityAudit::check(storage, cashPaymentRecordCount)', 'cash snapshot audit');
must(backup, 'return "crm_persistence_unstable_or_invalid";', 'fail-closed CRM backup reason');
must(backup, 'return "repair_delivery_unstable_or_invalid";', 'fail-closed delivery backup reason');
must(backup, 'return "cash_payment_unstable_or_invalid";', 'fail-closed cash backup reason');

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

must(cashStoreHeader, '/data/workshop/repair-payments.ndjson', 'cash journal path');
must(cashStore, 'FILE_APPEND', 'append-only cash evidence');
must(cashStore, 'kind == "PAYMENT"', 'payment kind');
must(cashStore, 'kind == "CORRECTION"', 'correction kind');
must(cashStore, 'direction == "ADD" || direction == "SUBTRACT"', 'correction directions');
must(cashStore, 'if (subtracted > added) return false;', 'non-negative paid total invariant');
if (cashStore.includes('client_price_minor')) {
  throw new Error('cash journal must not duplicate authoritative repair price');
}

must(cashWeb, '"/api/payments"', 'payment API');
must(cashWeb, '"/api/payments/balance"', 'balance API');
must(cashWeb, 'explicit_confirmation_required', 'payment explicit confirmation');
must(cashWeb, 'm_repairs.loadRepairIdentity', 'server-derived payment client identity');
must(cashWeb, 'm_costing.load(repairId, pricing)', 'authoritative repair price read');
must(cashWeb, 'event.currency != pricing.currency', 'payment currency guard');
must(cashWeb, 'correction_exceeds_paid_total', 'subtract correction lower bound');
must(cashWeb, 'charged_minor', 'charged balance field');
must(cashWeb, 'paid_minor', 'paid balance field');
must(cashWeb, 'debt_minor', 'debt balance field');
must(cashWeb, 'credit_minor', 'credit balance field');
if (cashWeb.includes('repairIsOpen')) {
  throw new Error('payments must remain possible after repair close/delivery');
}

must(cashAudit, 'CashPaymentStore::Path', 'cash audit journal source');
must(cashAudit, 'identity.clientId != clientId', 'cash repair/client identity audit');
must(cashAudit, 'pricing.currency != currency', 'cash pricing currency audit');
must(cashAudit, '!payments.eventExists(correctsId)', 'cash correction reference audit');
if (cashAudit.includes('storage.remove(')) {
  throw new Error('cash integrity audit must be read-only');
}

must(materialWeb, 'static CashPaymentStore cashPayments(SD);', 'cash payment production store');
must(materialWeb, 'static CashPaymentWeb cashPaymentWeb', 'cash payment production web');
must(materialWeb, 'cashRepairRegistry.begin() && cashPayments.begin()', 'cash runtime recovery before routes');

if (source.includes('m_storage.remove(') || source.includes('storage.remove(') ||
    deliveryAudit.includes('storage.remove(')) {
  throw new Error('CRM/delivery integrity audits must be read-only');
}
if (deliveryWeb.includes('m_payments') || deliveryWeb.includes('CashPaymentStore')) {
  throw new Error('delivery persistence must not require cash balance/payment state');
}

console.log('CRM backup/integrity + delivery + cash payment contracts OK');
