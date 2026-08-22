const fs = require('fs');

const storeHeader = fs.readFileSync('firmware/esp32/src/CM_WarehouseStore.h', 'utf8');
const productionStore = fs.readFileSync('firmware/esp32/src/CM_ConductorSettingsStore.cpp', 'utf8');
const web = fs.readFileSync('firmware/esp32/src/CM_ConductorSettingsWeb.cpp', 'utf8');
const integrity = fs.readFileSync('firmware/esp32/src/CM_ConductorSettingsIntegrityAudit.cpp', 'utf8');
const legacySourcePath = 'firmware/esp32/src/CM_ConductorSettings.cpp';
const legacyHeaderPath = 'firmware/esp32/src/CM_ConductorSettings.h';

function requireText(source, text, message) {
  if (!source.includes(text)) throw new Error(message);
}

requireText(web, 'm_store.setConversionSettings(settings)',
  'production calculator settings POST must use WarehouseStore');
requireText(web, 'm_store.loadConversionSettings(settings)',
  'production calculator settings GET must use WarehouseStore');

requireText(storeHeader, 'ConversionSettingsPath="/data/settings/conductor.json"',
  'production settings path must remain conductor.json');
requireText(storeHeader, 'ConversionSettingsTempPath="/data/settings/conductor.tmp"',
  'production settings transaction needs a temp path');
requireText(storeHeader, 'ConversionSettingsBackupPath="/data/settings/conductor.bak"',
  'production settings transaction needs a backup path');

requireText(productionStore, 'm_storage.open(ConversionSettingsTempPath, FILE_WRITE)',
  'production settings must write the verified temp first');
if (productionStore.includes('m_storage.open(ConversionSettingsPath, FILE_WRITE)')) {
  throw new Error('production settings must not overwrite conductor.json directly');
}
requireText(productionStore,
  'm_storage.rename(ConversionSettingsPath, ConversionSettingsBackupPath)',
  'existing committed settings must be preserved as backup before replacement');
requireText(productionStore,
  'm_storage.rename(ConversionSettingsTempPath, ConversionSettingsPath)',
  'verified temp must be promoted to the production path');
requireText(productionStore, 'if (backupExists)',
  'recovery must explicitly handle committed backup evidence');
requireText(productionStore, 'if (!backupValid) return false;',
  'invalid committed backup evidence must fail closed');
requireText(productionStore,
  'm_storage.remove(ConversionSettingsTempPath)',
  'prepared temp must be discarded when committed backup wins');
requireText(productionStore,
  'return m_storage.rename(ConversionSettingsBackupPath,\n                                ConversionSettingsPath);',
  'committed backup must be restored before considering prepared temp');

requireText(integrity, 'LegacySettingsPath = "/data/settings/conductor-calculator.ndjson"',
  'integrity audit must keep legacy persisted artefacts explicitly non-authoritative');
if (fs.existsSync(legacySourcePath) || fs.existsSync(legacyHeaderPath)) {
  throw new Error('legacy ConductorSettingsStore source returned; production owner must remain WarehouseStore');
}

console.log('Production conductor settings atomic recovery contracts OK');
