const fs = require('fs');

const header = fs.readFileSync('firmware/esp32/src/CM_RepairAsReceivedSnapshotStore.h', 'utf8');
const source = fs.readFileSync('firmware/esp32/src/CM_RepairAsReceivedSnapshotStore.cpp', 'utf8');

function requireText(haystack, needle, label) {
  if (!haystack.includes(needle)) {
    throw new Error(`missing ${label}: ${needle}`);
  }
}

requireText(header, '/data/workshop/repair-as-received.ndjson', 'append-only snapshot path');
requireText(header, 'uint32_t repairId;', 'repair provenance');
requireText(header, 'uint32_t clientId;', 'client provenance');
requireText(header, 'uint32_t motorId;', 'motor provenance');
requireText(header, 'uint32_t windingVersionId;', 'winding version provenance');
requireText(header, 'String workingProgram;', 'working program snapshot');
requireText(header, 'bool startingPresent;', 'starting role presence');
requireText(header, 'appendByRepairIdJson', 'read-by-repair API contract');

requireText(source, 'FILE_APPEND', 'append-only write mode');
requireText(source, 'currentRepairId <= previousRepairId', 'one ordered snapshot per repair guard');
requireText(source, 'if (found)', 'duplicate repair snapshot fail-closed lookup');
requireText(source, 'WindingProgramParser::canonicalize(snapshot.workingProgram', 'canonical working program');
requireText(source, '!snapshot.startingPresent &&', 'absent starting role validation');
requireText(source, 'snapshot.startingProgram.length() > 0U', 'starting role fail-closed validation');

for (const forbidden of ['FILE_WRITE', 'remove(Path)', 'rename(', 'truncate']) {
  if (source.includes(forbidden)) {
    throw new Error(`snapshot store must remain append-only; forbidden token: ${forbidden}`);
  }
}

console.log('repair as-received snapshot contract: OK');
