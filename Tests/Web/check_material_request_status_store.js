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

console.log('Material Request status-store contracts OK: resolve semantics preserved and mutation state plus next transition id share one status-journal pass.');
