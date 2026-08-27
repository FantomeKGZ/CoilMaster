const fs = require('fs');
const path = require('path');

const root = path.resolve(__dirname, '../..');
const headerPath = 'firmware/esp32/src/CM_CashPaymentStore.h';
const sourcePath = 'firmware/esp32/src/CM_CashPaymentStore.cpp';
const webPath = 'firmware/esp32/src/CM_CashPaymentWeb.cpp';

const header = fs.readFileSync(path.join(root, headerPath), 'utf8');
const source = fs.readFileSync(path.join(root, sourcePath), 'utf8');
const web = fs.readFileSync(path.join(root, webPath), 'utf8');
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

if (failures.length) {
  console.error(failures.join('\n'));
  process.exit(1);
}

console.log('Cash payment single-pass contracts OK: correction existence and next event id share one mutation-time journal pass while Web repair/client provenance preflight remains explicit.');
