const fs = require('fs');
const path = require('path');

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
console.log('Checked ' + files.length + ' HTML files and injected UI scripts: JavaScript, duplicate ids, internal links, and desktop navigation icons OK.');
