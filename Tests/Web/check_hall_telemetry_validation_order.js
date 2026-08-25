const fs = require('fs');
const path = require('path');

const root = path.resolve(__dirname, '../..');
const clientPath = path.join(root, 'firmware/esp32/src/CM_HardwareControlClient.cpp');
const clientCpp = fs.readFileSync(clientPath, 'utf8');

const handler = clientCpp.indexOf('bool HardwareControlClient::processTelemetryState');
const profileValidation = clientCpp.indexOf('parsed.hysteresis >= parsed.threshold', handler);
const validationReturn = clientCpp.indexOf('return true;', profileValidation);
const boundaryMath = clientCpp.indexOf('const uint16_t expectedReleaseBoundary', validationReturn);
const boundaryEquality = clientCpp.indexOf('parsed.releaseBoundary != expectedReleaseBoundary', boundaryMath);

if (handler < 0 || profileValidation < 0 || validationReturn < 0 ||
    boundaryMath < 0 || boundaryEquality < 0 ||
    !(handler < profileValidation && profileValidation < validationReturn &&
      validationReturn < boundaryMath && boundaryMath < boundaryEquality)) {
  throw new Error(
    'ESP32 HALL_STATE: validate threshold/hysteresis before release-boundary arithmetic and compare exact boundary afterwards');
}

console.log('Hall telemetry validation order contract: OK');
