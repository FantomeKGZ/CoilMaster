const fs = require('fs');
const path = require('path');

const root = path.resolve(__dirname, '../..');
const desktop = fs.readFileSync(path.join(root, 'firmware/esp32/web/desktop/arduino-windings.html'), 'utf8');
const mobile = fs.readFileSync(path.join(root, 'firmware/esp32/web/mobile/arduino-windings.html'), 'utf8');
const shared = fs.readFileSync(path.join(root, 'firmware/esp32/web/shared/arduino-windings-archive.js'), 'utf8');

function requireText(text, needle, label) {
  if (!text.includes(needle)) throw new Error(`${label}: missing ${needle}`);
}

for (const [label, page] of [['desktop', desktop], ['mobile', mobile]]) {
  requireText(page, '/shared/arduino-windings-archive.js', label);
  requireText(page, 'id="bulkAssign"', label);
  requireText(page, 'id="createSelected"', label);
  requireText(page, 'id="combineSelected"', label);
  requireText(page, 'id="historyScan"', label);
  requireText(page, 'id="selectedCount"', label);
}
requireText(desktop, 'class="table-wrap"', 'desktop compact table');
requireText(desktop, '.tip{', 'desktop accessible detail tooltip style');
requireText(shared, 'class="tip" tabindex="0" aria-label="Подробности"', 'desktop accessible detail tooltip markup');
requireText(shared, 'class="tip-box"', 'desktop accessible detail tooltip content');
requireText(mobile, 'details{', 'mobile accessible details style');
requireText(shared, '<details><summary>Подробности</summary>', 'mobile accessible details markup');

for (const needle of [
  '/api/autonomous-windings?',
  '/api/autonomous-windings/assign',
  '/api/motors?',
  '/api/motors',
  'session_id',
  'run_id',
  'repeat_target',
  'completed_runs',
  'historicalCounts',
  "status !== 'COMPLETED'"
]) requireText(shared, needle, 'shared archive controller');

requireText(shared, 'Физические RUN events не изменялись', 'immutable linkage wording');
requireText(shared, 'read-only scan архива', 'bounded historical scan wording');
requireText(shared, "return Number.isInteger(value) && value > 0 ? value : '—'", 'repeat target must not be guessed');

console.log('Arduino archive UI contracts OK');
