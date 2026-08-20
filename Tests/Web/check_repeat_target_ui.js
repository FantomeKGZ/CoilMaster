const fs = require('fs');
const path = require('path');

const root = path.resolve(__dirname, '../../firmware/esp32/web');
const pages = [
  'desktop/index.html',
  'mobile/index.html',
  'desktop/winding-job.html',
  'mobile/winding-job.html'
];

const failures = [];
for (const relative of pages) {
  const source = fs.readFileSync(path.join(root, relative), 'utf8');

  if (!source.includes('name="repeat_target"')) {
    failures.push(relative + ': repeat_target form field missing');
  }
  if (!source.includes('WAITING_NEXT_REPEAT')) {
    failures.push(relative + ': WAITING_NEXT_REPEAT UI state missing');
  }
  if (!source.includes('repeat_target')) {
    failures.push(relative + ': repeat_target status/response handling missing');
  }
  if (!source.includes('physical') && !source.includes('физичес')) {
    failures.push(relative + ': physical START requirement is not visible');
  }
}

for (const relative of ['desktop/winding-job.html', 'mobile/winding-job.html']) {
  const source = fs.readFileSync(path.join(root, relative), 'utf8');
  if (!source.includes('String(j.repeat_target)===String($(\'repeatTarget\').value)')) {
    failures.push(relative + ': linked-job response does not verify repeat_target');
  }
  if (!source.includes("$('liveRuns').textContent=String(s.completed_runs??0)+' / '+target")) {
    failures.push(relative + ': completed/target repeat progress missing');
  }
  if (!source.includes('max="65535"')) {
    failures.push(relative + ': UI repeat limit does not match uint16 backend limit');
  }
  if (!source.includes('каждый повтор') && !source.includes('Каждый повтор')) {
    failures.push(relative + ': per-repeat physical START wording missing');
  }
}

if (failures.length) {
  console.error(failures.join('\n'));
  process.exit(1);
}

console.log('Repeat-target web contracts OK: service and linked JOB UIs expose repeat_target, progress, WAITING_NEXT_REPEAT, and physical START semantics.');
