const fs = require('fs');
const path = require('path');
const vm = require('vm');

const root = path.resolve(__dirname, '../../firmware/esp32/web');
const files = [];
function walk(directory) {
  for (const entry of fs.readdirSync(directory, {withFileTypes: true})) {
    const full = path.join(directory, entry.name);
    if (entry.isDirectory()) walk(full);
    else if (entry.isFile() && /\.html?$/i.test(entry.name)) files.push(full);
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

  for (const match of html.matchAll(/href="([^"]+)"/g)) {
    const href = match[1];
    if (!href.startsWith('/') || href.startsWith('/api/') ||
        href.startsWith('//') || href.includes("'+")) continue;
    const target = href.split('#')[0].split('?')[0];
    if (target === '/' || routes.has(target)) continue;
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

if (failures.length) {
  console.error(failures.join('\n'));
  process.exit(1);
}
console.log('Checked ' + files.length + ' HTML files and injected UI scripts: JavaScript, duplicate ids, internal links, desktop navigation icons, and motor-import validation OK.');
