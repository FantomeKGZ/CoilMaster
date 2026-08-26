const fs = require('fs');

const source = fs.readFileSync('firmware/esp32/web/desktop/winding-job.html', 'utf8');
function must(needle, label) {
  if (!source.includes(needle)) throw new Error(`missing ${label}: ${needle}`);
}

must('id="roleSelect" name="type"', 'role selector');
must('/api/motors/winding/latest', 'latest version lookup');
must('working_program', 'WORKING program binding');
must('working_repeat_target', 'WORKING repeat binding');
must('starting_present===true', 'STARTING presence gate');
must('starting_program', 'STARTING program binding');
must('starting_repeat_target', 'STARTING repeat binding');
must("querySelector('option[value=\"starting\"]').disabled=!windingRoles.starting", 'STARTING option disable');
must("params.get('role')||'working'", 'role query support');
must("requestedRole==='starting'&&!windingRoles.starting", 'requested STARTING fail-closed branch');
must('Подмена на WORKING запрещена', 'no silent STARTING fallback');
must('id="turns" name="turns"', 'readonly turns field');
must('id="repeatTarget" name="repeat_target" type="number" min="1" max="65535" step="1" value="1" readonly required', 'readonly repeat field');
must('authoritative winding role', 'authoritative role wording');
must('exact role/program/repeat', 'server revalidation wording');
must("fetch('/api/jobs',{method:'POST',body:new FormData(e.target)})", 'existing linked job POST path');
must('физической кнопкой START', 'physical START safety wording');
must('RUN_COMPLETED не списывают провод автоматически', 'non-mutating completion wording');

if (source.includes('fetch(\'/api/hardware/')) throw new Error('role UI must not introduce direct hardware control');
if (source.includes('repeatTarget\').oninput')) throw new Error('linked repeat target must not remain operator-editable');

console.log('Winding job role UI contract OK: versioned WORKING/STARTING selection, fail-closed STARTING, readonly role program/repeat, existing linked POST path, and physical START/material safety are preserved.');
