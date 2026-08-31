const fs = require('fs');
const path = require('path');

const repoRoot = path.resolve(__dirname, '../..');
const webRoot = path.join(repoRoot, 'firmware/esp32/web');
const helper = fs.readFileSync(path.join(webRoot, 'shared/completed-job-display-reset.js'), 'utf8');
const desktop = fs.readFileSync(path.join(webRoot, 'desktop/index.html'), 'utf8');
const mobile = fs.readFileSync(path.join(webRoot, 'mobile/index.html'), 'utf8');
const failures = [];

for (const token of [
  'Последнее отправленное на Arduino',
  'Последнее выполненное на Arduino',
  "cm-last-completed-arduino-job-v1",
  'last_completed_job_id',
  'last_completed_program',
  'last_completed_repeat_target',
  'current_job_cleared_after_completion:true',
  "raw.job_status === 'PROGRAM_COMPLETED'",
  'authoritative RUN evidence',
  'localStorage.setItem(completedCacheKey'
]) {
  if (!helper.includes(token)) failures.push(`completed-job-display-reset.js: missing ${token}`);
}

for (const [name, source] of [['desktop/index.html', desktop], ['mobile/index.html', mobile]]) {
  if (!source.includes('/shared/completed-job-display-reset.js')) {
    failures.push(`${name}: completed-job display helper missing`);
  }
  if (!source.includes("fetch('/api/status'")) {
    failures.push(`${name}: dashboard status feed missing`);
  }
}

for (const forbidden of ['/api/ssr', 'automatic START', "fetch('/api/autonomous-windings'"]) {
  if (helper.includes(forbidden)) failures.push(`completed-job-display-reset.js: forbidden dashboard coupling ${forbidden}`);
}

if (failures.length) {
  console.error(failures.join('\n'));
  process.exit(1);
}

console.log('Dashboard Arduino job history contract OK: last sent/current status and browser-local last completed display remain read-only and physical START/authoritative RUN evidence are unchanged.');
