const fs = require('fs');

function read(path) {
  return fs.readFileSync(path, 'utf8');
}

function requireToken(source, token, label) {
  if (!source.includes(token)) {
    throw new Error(`missing ${label}: ${token}`);
  }
}

const header = read('firmware/esp32/src/CM_MaterialLedger.h');
const ledger = read('firmware/esp32/src/CM_MaterialLedger.cpp');
const state = read('firmware/esp32/src/CM_MaterialLedgerCurrency.cpp');
const swap = read('firmware/esp32/src/CM_MaterialLedgerSwap.cpp');
const web = read('firmware/esp32/src/CM_MaterialLedgerWeb.cpp');
const bridgeAudit = read('firmware/esp32/src/CM_SpoolMaterialBridgeIntegrityAudit.cpp');

for (const token of ['bool hasWireMetadata;', 'String wireType;', 'uint16_t diameterHundredthsMm;']) {
  requireToken(header, token, 'wire metadata state');
}

requireToken(ledger, 'if (material.hasWireMetadata)', 'writer metadata gate');
requireToken(ledger, 'material.unit != MaterialUnit::Gram', 'writer GRAM restriction');
requireToken(ledger, 'material.wireType != "CU" && material.wireType != "AL"', 'writer CU/AL restriction');
requireToken(ledger, '\\"wire_type\\":\\"', 'wire_type serialization');
requireToken(ledger, '\\"diameter_hundredths_mm\\":', 'diameter serialization');
requireToken(ledger, 'const bool hasWireType', 'list optional-pair validation');
requireToken(ledger, 'const bool hasDiameter', 'list optional-pair validation');
requireToken(ledger, 'if (hasWireType != hasDiameter)', 'list pair parity');
requireToken(ledger, 'if (unit != "GRAM" ||', 'list GRAM restriction');

requireToken(state, 'state.hasWireMetadata = hasWireType;', 'state metadata result');
requireToken(state, 'state.wireType = wireType;', 'state wire type result');
requireToken(state, 'state.diameterHundredthsMm = static_cast<uint16_t>(diameter);', 'state diameter result');
requireToken(state, 'if (hasWireType != hasDiameter ||', 'state pair parity');
requireToken(state, 'parsedUnit != MaterialUnit::Gram', 'state GRAM restriction');

requireToken(swap, 'const bool hasWireType', 'swap optional metadata');
requireToken(swap, 'if (hasWireType != hasDiameter)', 'swap pair parity');
requireToken(swap, 'if (unit != "GRAM" ||', 'swap GRAM restriction');

requireToken(web, 'hasWireType=m_server.hasArg("wire_type")', 'HTTP wire_type input');
requireToken(web, 'hasDiameter=m_server.hasArg("diameter_hundredths_mm")', 'HTTP diameter input');
requireToken(web, 'if(hasWireType!=hasDiameter)', 'HTTP wire metadata pair gate');
requireToken(web, 'wire_metadata_pair_required', 'HTTP pair error');
requireToken(web, 'invalid_wire_metadata', 'HTTP validation error');
requireToken(web, 'unit!=MaterialUnit::Gram', 'HTTP GRAM restriction');

// Backward compatibility: metadata remains optional. There must be no unconditional
// findString/findUnsigned requirement in the core required field parse before the pair gate.
if (!ledger.includes('const bool hasWireType = line.indexOf') ||
    !swap.includes('const bool hasWireType = line.indexOf') ||
    !state.includes('const bool hasWireType = line.indexOf')) {
  throw new Error('legacy/generic material records are no longer treated as optional-metadata records');
}

requireToken(bridgeAudit, '!findString(line, "unit", unit) || unit != "GRAM" ||', 'bridge material unit');
requireToken(bridgeAudit, '!findString(line, "wire_type", wireType) ||', 'bridge material wire type');
requireToken(bridgeAudit, 'wireType != reference.wireType ||', 'bridge exact wire type match');
requireToken(bridgeAudit, '!findUnsigned(line, "diameter_hundredths_mm", diameter) ||', 'bridge material diameter');
requireToken(bridgeAudit, 'diameter != reference.diameterHundredthsMm', 'bridge exact diameter match');

// This block must not silently turn bridge persistence into a runtime mutation path.
const sourceFiles = [ledger, state, swap, web, bridgeAudit].join('\n');
if (sourceFiles.includes('/api/warehouse/spool-material-bridges') ||
    sourceFiles.includes('/api/materials/spool-bridge')) {
  throw new Error('wire metadata block unexpectedly exposed a spool bridge mutation API');
}
if (sourceFiles.includes('RUN_COMPLETED') || sourceFiles.includes('HTTP_START')) {
  throw new Error('wire metadata block unexpectedly coupled to machine/run completion behavior');
}

console.log('MaterialLedger wire metadata contract audit passed');
require('./check_spool_material_bridge_web.js');
