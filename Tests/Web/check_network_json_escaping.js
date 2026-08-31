const fs = require('fs');

const networkWeb = fs.readFileSync('firmware/esp32/src/CM_NetworkWeb.cpp', 'utf8');
const networkManager = fs.readFileSync('firmware/esp32/src/CM_NetworkManager.cpp', 'utf8');
const networkStore = fs.readFileSync('firmware/esp32/src/CM_NetworkProfileStore.cpp', 'utf8');
const main = fs.readFileSync('firmware/esp32/src/main.cpp', 'utf8');
const staticSite = fs.readFileSync('firmware/esp32/src/CM_StaticSiteServer.cpp', 'utf8');
const wifiUi = fs.readFileSync('firmware/esp32/web/shared/settings-wifi.js', 'utf8');
const desktopWifi = fs.readFileSync('firmware/esp32/web/desktop/settings-wifi.html', 'utf8');
const mobileWifi = fs.readFileSync('firmware/esp32/web/mobile/settings-wifi.html', 'utf8');

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

for (const [needle, message] of [
  ['constexpr char LocalHostname[] = "coil";', 'canonical local hostname must remain coil'],
  ['mdnsReady = MDNS.begin(LocalHostname);', 'mDNS startup missing'],
  ['MDNS.addService("http", "tcp", 80U);', 'mDNS HTTP service advertisement missing'],
  ['staticSites.setLocalHostnameStatus(LocalHostname, mdnsReady);', 'mDNS runtime status propagation missing'],
]) requireText(main, needle, message);

for (const [needle, message] of [
  ['\\"mdns_supported\\":true,\\"mdns_active\\":', 'network status mDNS capability missing'],
  ['\\"local_hostname\\":\\"', 'network status local hostname missing'],
  ['\\"local_url\\":\\"http://', 'network status local URL missing'],
]) requireText(staticSite, needle, message);

for (const [needle, message] of [
  ['profile.useStaticIp', 'network manager static-IP branch missing'],
  ['WiFi.config(local, gateway, subnet, dns1, dns2)', 'static-IP configuration missing'],
  ['WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE)', 'DHCP reset path missing'],
]) requireText(networkManager, needle, message);
for (const [needle, message] of [
  ['sortProfiles(profiles, count);', 'profile priority ordering missing'],
  ['validIpv4(profile.localIp)', 'static local IPv4 validation missing'],
  ['validIpv4(profile.gateway)', 'static gateway validation missing'],
  ['validIpv4(profile.subnet)', 'static subnet validation missing'],
  ['validIpv4(profile.dns1, true)', 'optional DNS1 validation missing'],
  ['validIpv4(profile.dns2, true)', 'optional DNS2 validation missing'],
]) requireText(networkStore, needle, message);

for (const [needle, message] of [
  ['let items=[],saving=false;', 'Wi-Fi UI single-flight state missing'],
  ['if(saving)return;saving=true;', 'Wi-Fi profile save must reject a second in-flight submit'],
  ['if(submit)submit.disabled=true;', 'Wi-Fi profile submit control must lock while saving'],
  ['finally{saving=false;if(submit)submit.disabled=false}', 'Wi-Fi profile save lock must always be released'],
  ['networkStateText(s.network_state)', 'Wi-Fi runtime state must use localized presentation'],
  ['networkResultText(s.network_last_result)', 'Wi-Fi runtime result must use localized presentation'],
  ['function localAddressHtml(s)', 'Wi-Fi UI local hostname renderer missing'],
  ["s.mdns_active", 'Wi-Fi UI must honor actual mDNS runtime status'],
  ["s.local_hostname||'coil.local'", 'Wi-Fi UI canonical coil.local fallback missing'],
  ["s.local_url||('http://'+hostname+'/')", 'Wi-Fi UI local URL binding missing'],
  ['<span>Локальный адрес</span>', 'Wi-Fi runtime card must expose local hostname'],
  ["if(!s.mdns_active)return '<span class=\"muted\">недоступен; используйте IP</span>'", 'Wi-Fi UI must not falsely promise inactive mDNS'],
  ['id="useStaticIp"', 'static-IP toggle injection missing'],
  ['id="localIp"', 'static local-IP field missing'],
  ['id="gateway"', 'static gateway field missing'],
  ['id="subnet"', 'static subnet field missing'],
  ['id="dns1"', 'static DNS1 field missing'],
  ['id="dns2"', 'static DNS2 field missing'],
]) {
  requireText(wifiUi, needle, message);
}

for (const [name, source, variant] of [
  ['desktop/settings-wifi.html', desktopWifi, 'desktop'],
  ['mobile/settings-wifi.html', mobileWifi, 'mobile'],
]) {
  requireText(source, '/shared/settings-wifi.js', `${name}: shared Wi-Fi controller missing`);
  requireText(source, `CMWifiPage.start('${variant}')`, `${name}: shared Wi-Fi controller startup missing`);
  requireText(source, 'id="runtime"', `${name}: runtime status target missing`);
  requireText(source, 'id="form"', `${name}: profile form missing`);
  requireText(source, 'id="hidden"', `${name}: hidden-network field missing`);
  requireText(source, 'id="priority"', `${name}: priority field missing`);
}

console.log('Network contracts OK: JSON escaping, single-flight Wi-Fi UI, bounded profiles, DHCP/static IPv4, priority ordering and runtime coil.local mDNS visibility remain protected.');
