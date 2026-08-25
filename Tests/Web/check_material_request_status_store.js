const fs = require('fs');

const cpp = fs.readFileSync('firmware/esp32/src/CM_MaterialRequestStatusStore.cpp', 'utf8');
const header = fs.readFileSync('firmware/esp32/src/CM_MaterialRequestStatusStore.h', 'utf8');

function requireText(source, text, label) {
  if (!source.includes(text)) {
    throw new Error(`missing ${label}: ${text}`);
  }
}

requireText(header,
  'static constexpr const char* Path = "/data/workshop/material-request-status.ndjson";',
  'status journal path');
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

console.log('Material Request status-store contracts OK');
