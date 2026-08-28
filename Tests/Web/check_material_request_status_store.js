const fs = require('fs');

const cpp = fs.readFileSync('firmware/esp32/src/CM_MaterialRequestStatusStore.cpp', 'utf8');
const header = fs.readFileSync('firmware/esp32/src/CM_MaterialRequestStatusStore.h', 'utf8');

function requireText(source, text, label) {
  if (!source.includes(text)) {
    throw new Error(`missing ${label}: ${text}`);
  }
}

function forbidText(source, text, label) {
  if (source.includes(text)) {
    throw new Error(`forbidden ${label}: ${text}`);
  }
}

requireText(header,
  'static constexpr const char* Path = "/data/workshop/material-request-status.ndjson";',
  'status journal path');
requireText(header,
  'bool analyzeStatus(uint32_t materialRequestId,',
  'single-pass status analyzer declaration');
requireText(cpp,
  'return analyzeStatus(materialRequestId, state, found, nullptr);',
  'read-only resolve delegation');
requireText(cpp,
  'if (!analyzeStatus(materialRequestId, state, found, &transitionId) || !found ||',
  'transition state plus next-id single-pass preparation');
forbidText(header,
  'bool nextTransitionId(uint32_t& transitionId) const;',
  'standalone next-transition scan declaration');
forbidText(cpp,
  'MaterialRequestStatusStore::nextTransitionId(',
  'standalone next-transition scan implementation');

requireText(cpp,
  'if (!requestExists(materialRequestId, found)) return false;\n    if (!found) return true;',
  'missing request lookup semantics');
requireText(cpp,
  'state.status = "DRAFT";',
  'default DRAFT state');
requireText(cpp,
  '(fromStatus == "DRAFT" && toStatus == "ISSUED")',
  'DRAFT to ISSUED transition');
requireText(cpp,
  '(fromStatus == "ISSUED" && toStatus == "PRICED")',
  'ISSUED to PRICED transition');
requireText(cpp,
  '(fromStatus == "PRICED" && toStatus == "CLOSED")',
  'PRICED to CLOSED transition');
requireText(cpp,
  'if (fromStatus != state.status || state.transitionCount == 0xFFFFFFFFUL)',
  'per-request chain validation');
requireText(cpp,
  'transitionId <= previousTransitionId',
  'global transition ordering');
requireText(cpp,
  'if (previousTransitionId == 0xFFFFFFFFUL) return false;',
  'next transition id overflow guard');
requireText(cpp,
  '*nextTransitionId = previousTransitionId + 1UL;',
  'next transition id derivation inside status scan');

requireText(header,
  'static constexpr uint8_t MaxBatchSize = 24U;',
  'fixed bounded status batch size');
requireText(header,
  'bool resolveBatch(const uint32_t* materialRequestIds,',
  'bounded batch resolver declaration');
const batchStart = cpp.indexOf('bool MaterialRequestStatusStore::resolveBatch(');
const transitionStart = cpp.indexOf('bool MaterialRequestStatusStore::transition(', batchStart);
if (batchStart < 0 || transitionStart <= batchStart) {
  throw new Error('bounded batch resolver body missing');
}
const batch = cpp.slice(batchStart, transitionStart);
requireText(batch,
  'count == 0U || count > MaxBatchSize',
  'batch size fail-closed gate');
requireText(batch,
  'if (materialRequestIds[j] == materialRequestIds[i]) return false;',
  'duplicate request-id rejection');
requireText(batch,
  'File requests = m_storage.open(RequestsPath, FILE_READ);',
  'single request-journal batch scan');
requireText(batch,
  'File file = m_storage.open(Path, FILE_READ);',
  'single status-journal batch scan');
if ((batch.match(/m_storage\.open\(RequestsPath, FILE_READ\)/g) || []).length !== 1) {
  throw new Error('bounded batch resolver must scan request journal exactly once');
}
if ((batch.match(/m_storage\.open\(Path, FILE_READ\)/g) || []).length !== 1) {
  throw new Error('bounded batch resolver must scan status journal exactly once');
}
requireText(batch,
  'requestId <= previousRequestId',
  'global request ordering validation');
requireText(batch,
  'transitionId <= previousTransitionId',
  'global batch transition ordering validation');
requireText(batch,
  '!found[i] || fromStatus != states[i].status ||',
  'queried request chain fail-closed validation');
requireText(batch,
  'states[i].transitionCount == 0xFFFFFFFFUL',
  'batch transition-count overflow guard');
forbidText(batch, 'std::vector', 'unbounded batch vector');
forbidText(batch, 'readString()', 'whole-file batch buffering');

console.log('Material Request status-store contracts OK: single-item semantics preserved; bounded batch resolves up to 24 ids with one request scan and one status scan while keeping fail-closed chain validation.');
