const fs = require('fs');

function read(path) {
  return fs.readFileSync(path, 'utf8');
}

function requireToken(source, token, label) {
  if (!source.includes(token)) throw new Error(`missing ${label}: ${token}`);
}

const header = read('firmware/esp32/src/CM_SpoolMaterialBridgeWeb.h');
const web = read('firmware/esp32/src/CM_SpoolMaterialBridgeWeb.cpp');
const bootstrap = read('firmware/esp32/src/CM_WarehouseWeb.cpp');

requireToken(header, 'SpoolMaterialBridgeStore& bridges', 'bridge store dependency');
requireToken(web, '"/api/warehouse/spool-material-bridges", HTTP_POST', 'operator POST route');
requireToken(web, 'parseUnsigned(m_server, "confirm", 1UL, 1UL, confirm)', 'explicit confirmation');
requireToken(web, 'm_warehouse.loadActiveSpoolIdentity(spoolId, spool, spoolFound)', 'active spool preflight');
requireToken(web, 'm_materials.loadActiveMaterialState(warehouseItemId, material, materialFound)', 'active material preflight');
requireToken(web, 'material.unit != MaterialUnit::Gram || !material.hasWireMetadata', 'wire material contract');
requireToken(web, 'material.wireType != spool.wireType ||', 'wire type identity match');
requireToken(web, 'material.diameterHundredthsMm != spool.diameterHundredthsMm', 'diameter identity match');
requireToken(web, 'm_bridges.loadBySpool(spoolId, existing, bridgeFound)', 'duplicate bridge preflight');
requireToken(web, 'bridge.wireType = spool.wireType;', 'authoritative wire type source');
requireToken(web, 'bridge.diameterHundredthsMm = spool.diameterHundredthsMm;', 'authoritative diameter source');
requireToken(web, 'm_bridges.append(bridge, bridgeId)', 'append-only bridge mutation');
requireToken(web, '\\"stock_mutated\\":false', 'non-stock response contract');

for (const forbidden of [
  'confirmSpoolWriteOff(',
  'confirmKgFirstWriteOff(',
  'rewriteSpoolWeight(',
  'addSpool(',
  'addMaterial(',
  'RUN_COMPLETED',
  'HTTP_START'
]) {
  if (web.includes(forbidden)) throw new Error(`bridge API must not mutate production stock/machine state: ${forbidden}`);
}

requireToken(bootstrap, '#include "CM_SpoolMaterialBridgeWeb.h"', 'production bridge web include');
requireToken(bootstrap, 'static SpoolMaterialBridgeStore spoolMaterialBridgeStore(SD);', 'single bridge store bootstrap');
requireToken(bootstrap, 'static SpoolMaterialBridgeWeb spoolMaterialBridgeWeb(m_server, m_store, materialLedger, spoolMaterialBridgeStore);', 'bridge web ownership');
requireToken(bootstrap, 'spoolMaterialBridgeStore.begin();', 'bridge persistence initialization');
requireToken(bootstrap, 'spoolMaterialBridgeWeb.begin();', 'bridge route registration');

console.log('Operator spool-material bridge Web contract audit passed');
