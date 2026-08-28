const fs = require('fs');
const path = require('path');

const root = path.resolve(__dirname, '../..');
const headerPath = 'firmware/esp32/src/CM_CashPaymentStore.h';
const sourcePath = 'firmware/esp32/src/CM_CashPaymentStore.cpp';
const lookupPath = 'firmware/esp32/src/CM_CashPaymentStoreLookup.cpp';
const integrityPath = 'firmware/esp32/src/CM_CashPaymentIntegrityAudit.cpp';
const webPath = 'firmware/esp32/src/CM_CashPaymentWeb.cpp';
const costingHeaderPath = 'firmware/esp32/src/CM_RepairCosting.h';
const costingSourcePath = 'firmware/esp32/src/CM_RepairCosting.cpp';
const desktopPath = 'firmware/esp32/web/desktop/client-details.html';
const mobilePath = 'firmware/esp32/web/mobile/client-details.html';

const header = fs.readFileSync(path.join(root, headerPath), 'utf8');
const source = fs.readFileSync(path.join(root, sourcePath), 'utf8');
const lookup = fs.readFileSync(path.join(root, lookupPath), 'utf8');
const integrity = fs.readFileSync(path.join(root, integrityPath), 'utf8');
const web = fs.readFileSync(path.join(root, webPath), 'utf8');
const costingHeader = fs.readFileSync(path.join(root, costingHeaderPath), 'utf8');
const costingSource = fs.readFileSync(path.join(root, costingSourcePath), 'utf8');
const desktop = fs.readFileSync(path.join(root, desktopPath), 'utf8');
const mobile = fs.readFileSync(path.join(root, mobilePath), 'utf8');
const failures = [];

function requireText(relative, body, text, description) {
  if (!body.includes(text)) failures.push(`${relative}: ${description}: ${text}`);
}

function forbidText(relative, body, text, description) {
  if (body.includes(text)) failures.push(`${relative}: ${description}: ${text}`);
}

requireText(headerPath, header,
  'bool analyzeAppendState(uint32_t correctionEventId,',
  'single-pass append-state helper missing');
requireText(sourcePath, source,
  'bool CashPaymentStore::analyzeAppendState(uint32_t correctionEventId,',
  'single-pass append-state implementation missing');
requireText(sourcePath, source,
  'if (!analyzeAppendState(event.correctsEventId, eventId, correctionFound) ||',
  'append must derive correction presence and next id in one pass');
requireText(sourcePath, source,
  'if (id == correctionEventId) correctionFound = true;',
  'correction lookup must be fused into the next-id scan');
requireText(sourcePath, source,
  'if (previous == 0xFFFFFFFFUL) return false;',
  'next-id overflow guard must remain fail closed');
requireText(sourcePath, source,
  'eventId = previous + 1UL;',
  'next event id derivation missing');

forbidText(headerPath, header,
  'bool eventExists(uint32_t eventId) const;',
  'ambiguous bool-only event existence API must remain removed');
forbidText(sourcePath, source,
  'CashPaymentStore::eventExists(',
  'standalone correction existence scan must remain removed');
forbidText(sourcePath, source,
  'nextEventId(',
  'standalone next-id scan must remain removed');

// HTTP semantics remain richer than the mutation helper: the Web preflight must
// still validate exact repair/client ownership before append. Mutation then
// revalidates only existence + monotonic id allocation against current storage.
requireText(headerPath, header,
  'bool eventBelongsToRepair(uint32_t eventId,',
  'explicit repair/client correction lookup must remain public');
requireText(webPath, web,
  'm_payments.eventBelongsToRepair(event.correctsEventId,',
  'Web correction provenance preflight must remain intact');

// Client prepayment is intentionally a distinct client-level cash event. It
// must never acquire a repair provenance or silently reduce repair debt/costing.
requireText(headerPath, header,
  'uint64_t prepaymentMinor = 0ULL;',
  'client totals must expose a separate prepayment bucket');
requireText(headerPath, header,
  'uint32_t prepaymentEventCount = 0UL;',
  'client totals must expose separate prepayment event count');
requireText(sourcePath, source,
  'if (kind == "PAYMENT" || kind == "PREPAYMENT") return direction == "ADD";',
  'prepayment must be ADD-only');
requireText(sourcePath, source,
  'return kind == "PREPAYMENT" ? repairId == 0UL : repairId != 0UL;',
  'only PREPAYMENT may use repair_id=0');
requireText(sourcePath, source,
  'if (kind == "PREPAYMENT" || lineRepair != repairId) continue;',
  'repair totals/history must skip client prepayments');
requireText(lookupPath, lookup,
  'if (kind == "PREPAYMENT")',
  'client totals must branch prepayments away from repair payments');
requireText(lookupPath, lookup,
  '!addCashChecked(totals.prepaymentMinor, amount)',
  'prepayment total must be accumulated separately');
requireText(integrityPath, integrity,
  'const bool prepayment = kind == "PREPAYMENT";',
  'integrity audit must recognize prepayment explicitly');
requireText(integrityPath, integrity,
  'if (!repairs.clientExists(clientId, clientFound) || !clientFound)',
  'prepayment integrity must validate authoritative client existence');
requireText(webPath, web,
  'const bool prepayment = requestedKind == "PREPAYMENT";',
  'Web mutation must distinguish prepayment before repair preflight');
requireText(webPath, web,
  'if (m_server.hasArg("repair_id") || m_server.hasArg("corrects_event_id") ||',
  'Web must reject repair/correction provenance on prepayment');
requireText(webPath, web,
  'event.repairId = 0UL;',
  'persisted prepayment must remain client-scoped');
requireText(webPath, web,
  'response += F(",\\"prepayment_minor\\":");',
  'client balance must expose prepayment separately');
requireText(webPath, web,
  'const uint64_t debt = chargedMinor > paid.paidMinor',
  'client repair debt must remain based on repair payments only');

// Client-balance costing may reuse repair ids only after this request has done
// one authoritative full repairs.ndjson validation. Generic and mutation paths
// must keep the normal exact repairExists() guard.
requireText(costingHeaderPath, costingHeader,
  'bool loadKnownRepair(uint32_t repairId, RepairCostSummary& summary) const;',
  'known-repair read-only costing path missing');
requireText(costingSourcePath, costingSource,
  'if (!repairExists(repairId, repairFound) || !repairFound) return false;',
  'generic costing load must retain authoritative repair existence validation');
requireText(costingSourcePath, costingSource,
  'return loadKnownRepair(repairId, summary);',
  'generic costing load must delegate only after repair existence validation');
requireText(costingSourcePath, costingSource,
  'bool RepairCosting::loadKnownRepair(uint32_t repairId, RepairCostSummary& summary) const',
  'known-repair costing implementation missing');
requireText(webPath, web,
  'bool repairsValidated = false;',
  'client balance must track one full repair-journal validation per request');
requireText(webPath, web,
  'if (idCount > 0U && !repairsValidated)',
  'client balance must gate authoritative validation before known-repair loads');
requireText(webPath, web,
  'if (!m_costing.repairExists(repairIds[0], repairFound) || !repairFound)',
  'client balance must perform the authoritative repair-journal validation');
requireText(webPath, web,
  'repairsValidated = true;',
  'client balance must mark the repair journal validated only after success');
requireText(webPath, web,
  'if (!m_costing.loadKnownRepair(repairIds[i], pricing)) return false;',
  'client balance must reuse already validated repair identities');
forbidText(webPath, web,
  'if (!m_costing.load(repairIds[i], pricing)) return false;',
  'client balance must not repeat full repairs.ndjson validation for every repair');

for (const [relative, body] of [[desktopPath, desktop], [mobilePath, mobile]]) {
  requireText(relative, body, '/api/clients/update',
    'client card must expose append-only client editing');
  requireText(relative, body, "kind:'PREPAYMENT'",
    'client card must submit an explicit PREPAYMENT kind');
  requireText(relative, body, "confirmed:'true'",
    'client prepayment must require explicit confirmation');
  requireText(relative, body, 'prepayment_minor',
    'client card must display separate prepayment balance');
  requireText(relative, body, 'Зачислить предоплату',
    'client card must label the prepayment action explicitly');
}

if (failures.length) {
  console.error(failures.join('\n'));
  process.exit(1);
}

console.log('Cash payment contracts OK: correction/prepayment invariants remain intact; client balance validates repairs.ndjson once per request before reusing known repair ids for costing.');
