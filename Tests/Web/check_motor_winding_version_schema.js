'use strict';

const fs = require('fs');

const header = fs.readFileSync('firmware/esp32/src/CM_MotorWindingVersionStore.h', 'utf8');
const source = fs.readFileSync('firmware/esp32/src/CM_MotorWindingVersionStore.cpp', 'utf8');

function requireText(text, needle, label) {
  if (!text.includes(needle)) {
    throw new Error(`${label}: missing ${needle}`);
  }
}

requireText(header, 'motor-winding-versions.ndjson', 'append-only store path');
requireText(header, 'MaxConductors = 4U', 'bounded conductor count');
requireText(header, 'MotorWindingRoleSpec working;', 'working role');
requireText(header, 'MotorWindingRoleSpec starting;', 'starting role');
requireText(header, 'previousVersionId', 'predecessor linkage');
requireText(header, 'sourceRepairId', 'repair linkage');
requireText(source, 'version.working.present', 'working role required');
requireText(source, 'value == "CU" || value == "AL"', 'wire material classes');
requireText(source, 'WindingProgramParser::canonicalize', 'program canonicalization');
requireText(source, 'canonical += conductor.diameterHundredthsMm;', 'conductor diameter serialization');
requireText(source, "canonical += 'x';", 'conductor strand serialization');
requireText(source, 'current <= previous', 'monotonic append-only id validation');
requireText(source, "file.read() != '\\n'", 'newline-complete NDJSON guard');

if (source.includes('spool_id') || header.includes('spool_id')) {
  throw new Error('motor winding version schema must not couple winding definition to spool inventory');
}

console.log('Motor winding version schema contracts OK');
