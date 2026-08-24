const fs = require('fs');
const path = require('path');

const root = path.resolve(__dirname, '../..');
const client = fs.readFileSync(
  path.join(root, 'firmware/esp32/src/CM_HardwareControlClient.cpp'),
  'utf8'
);

const waiting = client.indexOf('const bool proposalWasWaitingApply');
const completed = client.indexOf(
  'parsed.state == HallCalibrationRemoteState::Completed', waiting
);
const cfgGet = client.indexOf('CMP1|CFG_GET|HALL|C', completed);
const switchToSettings = client.indexOf(
  'm_requestType = RequestType::GetSettings;', cfgGet
);
const noImmediateTimeoutEnd = client.indexOf('return true;', switchToSettings);

if (waiting < 0 || completed < 0 || cfgGet < 0 || switchToSettings < 0 ||
    noImmediateTimeoutEnd < 0 ||
    !(waiting < completed && completed < cfgGet && cfgGet < switchToSettings &&
      switchToSettings < noImmediateTimeoutEnd)) {
  throw new Error(
    'Lost CAL_APPLIED must switch the staged proposal to authoritative CFG_GET reconciliation'
  );
}

const settingsHandler = client.indexOf(
  'bool HardwareControlClient::processSettingsState'
);
const pendingIdentity = client.indexOf(
  'm_pendingCalibrationMeasurementId != 0UL', settingsHandler
);
const reconcileTimeout = client.indexOf(
  'finishRequest(HardwareControlReplyResult::TimedOut)', pendingIdentity
);
const mirrorWrite = client.indexOf('m_settings = parsed;', settingsHandler);

if (settingsHandler < 0 || mirrorWrite < 0 || pendingIdentity < 0 ||
    reconcileTimeout < 0 ||
    !(settingsHandler < mirrorWrite && mirrorWrite < pendingIdentity &&
      pendingIdentity < reconcileTimeout)) {
  throw new Error(
    'EEPROM reconciliation must update the authoritative mirror before preserving ambiguous timeout result'
  );
}

const proposalBlockEnd = client.indexOf(
  'bool HardwareControlClient::requestPending() const'
);
const proposalBlock = client.slice(waiting, proposalBlockEnd > waiting ? proposalBlockEnd : undefined);
if (proposalBlock.includes('sendPending(nowMs)')) {
  throw new Error('Staged apply reconciliation must not directly replay CAL_PROPOSAL');
}

console.log('Hall lost-apply reconciliation contract: OK');
