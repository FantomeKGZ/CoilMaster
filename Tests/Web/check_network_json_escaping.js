const fs = require('fs');

const networkWeb = fs.readFileSync('firmware/esp32/src/CM_NetworkWeb.cpp', 'utf8');
const staticSite = fs.readFileSync('firmware/esp32/src/CM_StaticSiteServer.cpp', 'utf8');

function requireText(source, needle, message) {
  if (!source.includes(needle)) throw new Error(message);
}

function requireJsonEscapeShape(source, helperName) {
  requireText(source, "case '\"': result += F(\"\\\\\\\"\"); break;",
    `${helperName} must escape JSON quotes`);
  requireText(source, "case '\\\\': result += F(\"\\\\\\\\\"); break;",
    `${helperName} must escape JSON backslashes`);
  requireText(source, "case '\\n': result += F(\"\\\\n\"); break;",
    `${helperName} must escape JSON newlines`);
  requireText(source, "case '\\r': result += F(\"\\\\r\"); break;",
    `${helperName} must escape JSON carriage returns`);
  requireText(source, "case '\\t': result += F(\"\\\\t\"); break;",
    `${helperName} must escape JSON tabs`);
  requireText(source, 'if (byte < 0x20U)',
    `${helperName} must reject raw JSON control bytes`);
  requireText(source, 'result += F("\\\\u00");',
    `${helperName} must encode remaining JSON control bytes as unicode escapes`);
}

requireJsonEscapeShape(networkWeb, 'NetworkWeb::escaped');
requireText(networkWeb, 'response += escaped(profiles[i].ssid);',
  'saved network profile SSID must use JSON escaping');
requireText(networkWeb, 'response += escaped(WiFi.SSID(index));',
  'network scan SSID must use JSON escaping');

requireJsonEscapeShape(staticSite, 'StaticSiteServer jsonEscaped');
for (const expression of [
  'jsonEscaped(m_networkManager.stateName())',
  'jsonEscaped(m_networkManager.lastResult())',
  'jsonEscaped(WiFi.softAPSSID())',
  'jsonEscaped(WiFi.SSID())',
  'jsonEscaped(m_localHostname)',
]) {
  requireText(staticSite, expression,
    `/api/system/network string field is not protected by JSON escaping: ${expression}`);
}

console.log('Network JSON escaping contracts OK');
