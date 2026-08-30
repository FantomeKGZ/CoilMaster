const fs = require('fs');
const path = require('path');
const vm = require('vm');

const root = path.resolve(__dirname, '../../firmware/esp32/web');
const files = [];
const staticFiles = new Set();
function walk(directory) {
  for (const entry of fs.readdirSync(directory, {withFileTypes: true})) {
    const full = path.join(directory, entry.name);
    if (entry.isDirectory()) walk(full);
    else if (entry.isFile()) {
      const relative = '/' + path.relative(root, full).split(path.sep).join('/');
      staticFiles.add(relative);
      if (/\.html?$/i.test(entry.name)) files.push(full);
    }
  }
}
walk(root);

const routes = new Set(['/desktop', '/desktop/', '/mobile', '/mobile/']);
for (const file of files) {
  const relative = '/' + path.relative(root, file).split(path.sep).join('/');
  routes.add(relative);
  if (relative.endsWith('/index.html')) {
    routes.add(relative.slice(0, -'index.html'.length));
    routes.add(relative.slice(0, -'/index.html'.length));
  }
}

function staticTargetExists(target) {
  let decoded;
  try {
    decoded = decodeURIComponent(target);
  } catch (_) {
    decoded = target;
  }
  const normalized = '/' + decoded.replace(/^\/+/, '').replace(/\\/g, '/');
  if (staticFiles.has(normalized)) return true;
  const indexTarget = normalized.replace(/\/$/, '') + '/index.html';
  return staticFiles.has(indexTarget);
}

const failures = [];
for (const entry of fs.readdirSync(path.join(root, 'shared'))) {
  if (!entry.endsWith('.js')) continue;
  const relative = 'shared/' + entry;
  try {
    new Function(fs.readFileSync(path.join(root, relative), 'utf8'));
  } catch (error) {
    failures.push(relative + ': JavaScript syntax: ' + error.message);
  }
}
for (const file of files) {
  const relative = path.relative(root, file).split(path.sep).join('/');
  const html = fs.readFileSync(file, 'utf8');

  for (const match of html.matchAll(/<script(?:\s[^>]*)?>([\s\S]*?)<\/script>/gi)) {
    if (!match[1].trim()) continue;
    try {
      new Function(match[1]);
    } catch (error) {
      failures.push(relative + ': embedded JavaScript syntax: ' + error.message);
    }
  }

  const ids = new Set();
  for (const match of html.matchAll(/\sid="([^"]+)"/g)) {
    if (ids.has(match[1])) failures.push(relative + ': duplicate id "' + match[1] + '"');
    ids.add(match[1]);
  }

  for (const match of html.matchAll(/<a\b([^>]*)\bhref="([^"]+)"([^>]*)>/gi)) {
    const href = match[2];
    if (!href.startsWith('/') || href.startsWith('/api/') ||
        href.startsWith('//') || href.includes("'+")) continue;
    const target = href.split('#')[0].split('?')[0];
    if (target === '/' || routes.has(target) || staticTargetExists(target)) continue;

    const attributes = match[1] + match[3];
    const generatedReferenceTarget =
      /\bdata-reference-direct\b/i.test(attributes) &&
      /^\/sites\/reference\/(desktop|mobile)\/pages\/[^/]+\.html?$/i.test(target);
    if (generatedReferenceTarget) continue;

    failures.push(relative + ': missing internal link target ' + href);
  }
}

function auditMotorImport(relative) {
  const html = fs.readFileSync(path.join(root, relative), 'utf8');
  const scripts = [...html.matchAll(/<script(?:\s[^>]*)?>([\s\S]*?)<\/script>/gi)];
  if (scripts.length !== 1) {
    failures.push(relative + ': expected one embedded import script');
    return;
  }
  const elements = new Map();
  const element = id => {
    if (!elements.has(id)) elements.set(id, {
      id, value: '', textContent: '', className: '', disabled: false,
      files: [], onclick: null, onchange: null
    });
    return elements.get(id);
  };
  const context = vm.createContext({
    document: {getElementById: element, querySelectorAll: () => []},
    URL, URLSearchParams, Date, FormData: class { set() {} },
    fetch: async () => { throw new Error('unexpected fetch'); },
    confirm: () => false, console
  });
  try {
    vm.runInContext(scripts[0][1], context, {filename: relative});
    const run = expression => vm.runInContext(expression, context);
    const valid = run('JSON.parse(JSON.stringify(example[0]))');
    let errors = run('validate')(valid);
    if (errors.length) failures.push(relative + ': documented example rejected: ' + errors.join('; '));
    if (valid.name !== 'АИР 80A2' || valid.coil_program !== '120/120/120') {
      failures.push(relative + ': valid import normalization changed unexpectedly');
    }

    const unknown = run('JSON.parse(JSON.stringify(example[0]))');
    unknown.slot_counts = 24;
    errors = run('validate')(unknown);
    if (!errors.some(x => x.includes('неизвестное поле slot_counts'))) {
      failures.push(relative + ': unknown import field is not rejected');
    }

    const badDate = run('JSON.parse(JSON.stringify(example[0]))');
    badDate.source_retrieved_at = '2026-02-30';
    errors = run('validate')(badDate);
    if (!errors.some(x => x.includes('source_retrieved_at'))) {
      failures.push(relative + ': invalid source date is not rejected');
    }

    const calculated = run('JSON.parse(JSON.stringify(example[0]))');
    calculated.confidence = 'CALCULATED';
    errors = run('validate')(calculated);
    if (!errors.some(x => x.includes('calculated_fields'))) {
      failures.push(relative + ': calculated provenance mismatch is not rejected');
    }
    calculated.calculated_fields = true;
    errors = run('validate')(calculated);
    if (errors.length) failures.push(relative + ': valid calculated record rejected: ' + errors.join('; '));

    const duplicateA = run('JSON.parse(JSON.stringify(example[0]))');
    const duplicateB = run('JSON.parse(JSON.stringify(example[0]))');
    duplicateB.name = duplicateB.name.toLowerCase();
    if (!run('packageIdentityMatch')(duplicateA, duplicateB)) {
      failures.push(relative + ': duplicate records inside one package are not matched');
    }
  } catch (error) {
    failures.push(relative + ': executable import audit: ' + error.message);
  }
}
auditMotorImport('desktop/motor-import.html');
auditMotorImport('mobile/motor-import.html');

const staticSiteServerPath = path.resolve(
  __dirname, '../../firmware/esp32/src/CM_StaticSiteServer.cpp');
const staticSiteServer = fs.readFileSync(staticSiteServerPath, 'utf8');
const injectedScripts = [...staticSiteServer.matchAll(
  /R"HTML\(\s*<script>([\s\S]*?)<\/script>\s*\)HTML"/g)];
if (!injectedScripts.length) {
  failures.push('CM_StaticSiteServer.cpp: injected HTML script not found');
}
for (const match of injectedScripts) {
  try {
    new Function(match[1]);
  } catch (error) {
    failures.push('CM_StaticSiteServer.cpp: injected JavaScript syntax: ' +
                  error.message);
  }
}
if (!staticSiteServer.includes("querySelectorAll('aside a')") ||
    !staticSiteServer.includes('cm-nav-icon')) {
  failures.push('CM_StaticSiteServer.cpp: desktop navigation icon normalizer missing');
}

const mainPath = path.resolve(__dirname, '../../firmware/esp32/src/main.cpp');
const mainSource = fs.readFileSync(mainPath, 'utf8');
if (!mainSource.includes('webServer.on("/api/system/diagnostics"') ||
    !mainSource.includes('ESP_RST_BROWNOUT') ||
    !mainSource.includes('brownout_reset_detected')) {
  failures.push('main.cpp: read-only ESP32 reset/power diagnostics missing');
}
if (!staticSiteServer.includes('/shared/settings-system-diagnostics.js')) {
  failures.push('CM_StaticSiteServer.cpp: settings system diagnostics injection missing');
}

const storageDiagnosticsPath = path.resolve(
  __dirname, '../../firmware/esp32/src/CM_StorageDiagnosticsWeb.cpp');
const storageDiagnosticsHeaderPath = path.resolve(
  __dirname, '../../firmware/esp32/src/CM_StaticSiteServer.h');
const storageDiagnosticsUiPath = path.resolve(
  __dirname, '../../firmware/esp32/web/shared/settings-system-diagnostics.js');
if (!fs.existsSync(storageDiagnosticsPath)) {
  failures.push('CM_StorageDiagnosticsWeb.cpp: read-only microSD diagnostics module missing');
} else {
  const source = fs.readFileSync(storageDiagnosticsPath, 'utf8');
  if (!source.includes('/api/system/storage') ||
      !source.includes('cardSize()') ||
      !source.includes('totalBytes()') ||
      !source.includes('usedBytes()') ||
      !source.includes('automatic_cleanup_allowed\\\":false')) {
    failures.push('CM_StorageDiagnosticsWeb.cpp: required microSD capacity diagnostics missing');
  }
  if (source.includes('.remove(') || source.includes('.rename(') ||
      source.includes('FILE_WRITE') || source.includes('FILE_APPEND')) {
    failures.push('CM_StorageDiagnosticsWeb.cpp: storage diagnostics must remain read-only');
  }
}
if (!fs.readFileSync(storageDiagnosticsHeaderPath, 'utf8').includes(
      'StorageDiagnosticsWeb m_storageDiagnosticsWeb{m_server, m_storage};')) {
  failures.push('CM_StaticSiteServer.h: microSD diagnostics registration missing');
}
const storageDiagnosticsUi = fs.readFileSync(storageDiagnosticsUiPath, 'utf8');
if (!storageDiagnosticsUi.includes('/api/system/storage') ||
    !storageDiagnosticsUi.includes('filesystem_free_bytes') ||
    !storageDiagnosticsUi.includes('Свободно на microSD') ||
    !storageDiagnosticsUi.includes('автоматическая очистка и ротация рабочих данных отключены')) {
  failures.push('settings-system-diagnostics.js: microSD free-space diagnostics UI missing');
}

const remoteBackupWebPath = path.resolve(
  __dirname, '../../firmware/esp32/src/CM_RemoteBackupWeb.cpp');
const remoteBackupWeb = fs.readFileSync(remoteBackupWebPath, 'utf8');
const applyActiveStart = remoteBackupWeb.indexOf(
  'bool RemoteBackupWeb::applyActive() const');
const applyActiveEnd = remoteBackupWeb.indexOf(
  'uint32_t RemoteBackupWeb::dateKey', applyActiveStart);
const applyActiveSource = applyActiveStart >= 0 && applyActiveEnd > applyActiveStart
  ? remoteBackupWeb.slice(applyActiveStart, applyActiveEnd) : '';
if (!applyActiveSource.includes('m_storage.exists(ApplyJournalPath)') ||
    !applyActiveSource.includes('m_storage.exists(ApplyResultMarkerPath)')) {
  failures.push('CM_RemoteBackupWeb.cpp: persisted apply evidence is not part of backend busy lock');
}

const cleanupStart = remoteBackupWeb.indexOf(
  'void RemoteBackupWeb::handleDiscardStaging()');
const cleanupEnd = remoteBackupWeb.indexOf(
  'void RemoteBackupWeb::handleStartRestorePlan()', cleanupStart);
const cleanupSource = cleanupStart >= 0 && cleanupEnd > cleanupStart
  ? remoteBackupWeb.slice(cleanupStart, cleanupEnd) : '';
if (!cleanupSource.includes('runtimeApplyActive') ||
    cleanupSource.includes('applyPreflightActive() || applyActive()')) {
  failures.push('CM_RemoteBackupWeb.cpp: explicit stale cleanup no longer has runtime-only apply guard');
}

const applyStatusStart = remoteBackupWeb.indexOf(
  'void RemoteBackupWeb::handleApplyStatus()');
const applyStatusEnd = remoteBackupWeb.indexOf(
  'bool RemoteBackupWeb::validateInspectionManifest', applyStatusStart);
const applyStatusSource = applyStatusStart >= 0 && applyStatusEnd > applyStatusStart
  ? remoteBackupWeb.slice(applyStatusStart, applyStatusEnd) : '';
if (!applyStatusSource.includes('state = "STALE"') ||
    !applyStatusSource.includes('response += runtimeApplyActive ? F("true") : F("false")')) {
  failures.push('CM_RemoteBackupWeb.cpp: post-reboot STALE status must remain inactive runtime evidence');
}

const scheduleStart = remoteBackupWeb.indexOf(
  'void RemoteBackupWeb::updateSchedule(uint32_t nowMs)');
const scheduleEnd = remoteBackupWeb.indexOf(
  'void RemoteBackupWeb::completeScheduledBatch()', scheduleStart);
const scheduleSource = scheduleStart >= 0 && scheduleEnd > scheduleStart
  ? remoteBackupWeb.slice(scheduleStart, scheduleEnd) : '';
if (!scheduleSource.includes('m_storage.exists(ApplyJournalPath)') ||
    !scheduleSource.includes('m_storage.exists(ApplyResultMarkerPath)') ||
    !scheduleSource.includes('WAITING_RESTORE_CLEANUP')) {
  failures.push('CM_RemoteBackupWeb.cpp: scheduler restore-cleanup wait gate missing');
}
if (scheduleSource.indexOf('WAITING_RESTORE_CLEANUP') >
    scheduleSource.indexOf('m_scheduleAttemptDate = today')) {
  failures.push('CM_RemoteBackupWeb.cpp: scheduler consumes daily attempt before restore cleanup gate');
}

if (failures.length) {
  console.error(failures.join('\n'));
  process.exit(1);
}
console.log('Checked ' + files.length + ' HTML files and injected UI scripts: JavaScript, duplicate ids, internal links/assets, desktop navigation icons, motor-import validation, microSD diagnostics, and restore stale-lock contracts OK.');
