const fs = require('fs');

const requestHeader = fs.readFileSync('firmware/esp32/src/CM_MaterialRequestStore.h', 'utf8');
const requestSource = fs.readFileSync('firmware/esp32/src/CM_MaterialRequestStore.cpp', 'utf8');
const movementHeader = fs.readFileSync('firmware/esp32/src/CM_MaterialRequestMovementStore.h', 'utf8');
const movementSource = fs.readFileSync('firmware/esp32/src/CM_MaterialRequestMovementStore.cpp', 'utf8');

function must(text, needle, label) {
  if (!text.includes(needle)) throw new Error(`missing ${label}: ${needle}`);
}

must(requestHeader, '/data/workshop/material-requests.ndjson', 'material request store path');
must(requestHeader, 'uint32_t repairId;', 'repair provenance');
must(requestHeader, 'uint32_t clientId;', 'client provenance');
must(requestHeader, 'uint32_t motorId;', 'motor provenance');
must(requestSource, '\\"initial_status\\":\\"DRAFT\\"', 'immutable draft creation');
must(requestSource, 'FILE_APPEND', 'append-only request persistence');
must(requestSource, 'material_request_id', 'monotonic request identity');

must(movementHeader, '/data/workshop/material-request-movements.ndjson', 'movement store path');
must(movementHeader, 'ISSUE | RETURN | CORRECTION', 'movement kinds contract');
must(movementHeader, 'MANUAL_MATERIAL | RUN_WIRE', 'movement source contract');
must(movementHeader, 'uint32_t sourceSessionId;', 'session provenance');
must(movementHeader, 'uint32_t sourceRunId;', 'run provenance');
must(movementHeader, 'uint32_t quantityMilliUnits;', 'integer quantity representation');
must(movementHeader, 'uint64_t unitCostMinor;', 'unit cost snapshot');
must(movementHeader, 'uint64_t costAmountMinor;', 'line cost snapshot');
must(movementSource, 'movement.movementKind != "ISSUE"', 'ISSUE validation');
must(movementSource, 'movement.movementKind != "RETURN"', 'RETURN validation');
must(movementSource, 'movement.movementKind != "CORRECTION"', 'CORRECTION validation');
must(movementSource, 'movement.sourceKind == "RUN_WIRE"', 'run wire branch');
must(movementSource, 'movement.sourceSessionId > 0UL && movement.sourceRunId > 0UL', 'exact run pair');
must(movementSource, 'movement.materialClass == "CU" || movement.materialClass == "AL"', 'wire material class');
must(movementSource, 'movement.unit == "KG"', 'wire weight unit');
must(movementSource, 'movement.sourceKind != "MANUAL_MATERIAL"', 'manual material branch');
must(movementSource, 'movement.sourceSessionId == 0UL && movement.sourceRunId == 0UL', 'manual material no fake run provenance');
must(movementSource, 'FILE_APPEND', 'append-only movement persistence');

if (movementSource.includes('RUN_COMPLETED')) {
  throw new Error('material request movement persistence must not couple itself to RUN_COMPLETED auto-writeoff');
}

console.log('material request schema contracts OK');
