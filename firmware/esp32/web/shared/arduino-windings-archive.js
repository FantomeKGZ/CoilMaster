(() => {
'use strict';
const $ = id => document.getElementById(id);
const esc = value => String(value ?? '').replace(/[&<>"']/g, c => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
const PAGE = 20;
const ARCHIVE_PAGE = 32;
const view = document.body.dataset.view === 'mobile' ? 'mobile' : 'desktop';
localStorage.setItem('cm-ui-version', view);

let motors = [];
let motorsLoaded = false;
let motorsLoading = false;
let tasks = [];
let nextCursor = null;
let hasMore = false;
let loading = false;
let historyLoading = false;
const selected = new Set();
const historicalCounts = new Map();

function clampTolerance() {
  return Math.max(0, Math.min(50, Number($('tolerance').value) || 20));
}
function taskKey(task) { return `${task.session_id}:${task.run_id}`; }
function parseProgram(program) {
  const values = String(program || '').trim().split(/[\/,;]/).map(Number);
  return values.length && values.every(v => Number.isInteger(v) && v > 0) ? values : null;
}
function normalizedProgram(program) {
  const parsed = parseProgram(program);
  return parsed ? parsed.join('/') : String(program || '').trim();
}
function motorLabel(motor) {
  const title = [motor.manufacturer, motor.model].filter(Boolean).join(' ');
  return `${title || motor.name || `Двигатель №${motor.motor_id}`} · ${motor.coil_program || 'без программы'}`;
}
function queryFor(cursor, program, tolerance, limit) {
  const query = new URLSearchParams({tolerance_percent:String(tolerance), limit:String(limit)});
  if (program) query.set('program', program);
  if (cursor) query.set('cursor', String(cursor));
  return query;
}
async function jsonFetch(url, options) {
  const response = await fetch(url, options);
  let body = {};
  try { body = await response.json(); } catch (_) {}
  if (!response.ok) throw new Error(body.error || `HTTP_${response.status}`);
  return body;
}
async function fetchPage(cursor, program, tolerance, limit) {
  return jsonFetch('/api/autonomous-windings?' + queryFor(cursor, program, tolerance, limit), {cache:'no-store'});
}
async function loadMotors() {
  if (motorsLoaded || motorsLoading) return;
  motorsLoading = true;
  const select = $('motorSelect');
  const previous = select.value;
  select.disabled = true;
  select.innerHTML = '<option value="">Загрузка двигателей…</option>';
  try {
    const items = [];
    let cursor = 0;
    for (;;) {
      const q = new URLSearchParams({limit:'32'});
      if (cursor) q.set('cursor', String(cursor));
      const page = await jsonFetch('/api/motors?' + q, {cache:'no-store'});
      items.push(...(page.items || []));
      if (page.has_more !== true) break;
      const next = Number(page.next_cursor);
      if (!Number.isInteger(next) || next <= cursor) throw new Error('invalid_motor_cursor');
      cursor = next;
    }
    motors = items;
    motorsLoaded = true;
    renderMotorOptions(previous);
    renderTasks();
  } catch (error) {
    select.innerHTML = '<option value="">Не удалось загрузить двигатели</option>';
    setBulkState('Ошибка загрузки двигателей: ' + error.message, 'note warn');
  } finally {
    motorsLoading = false;
    select.disabled = false;
    updateBulkControls();
  }
}
function renderMotorOptions(previous) {
  const select = $('motorSelect');
  const current = previous ?? select.value;
  if (!motorsLoaded) {
    select.innerHTML = '<option value="">Выберите для загрузки двигателей…</option>';
    return;
  }
  select.innerHTML = '<option value="">Выберите двигатель…</option>' + motors.map(m =>
    `<option value="${Number(m.motor_id)}">${esc(motorLabel(m))}</option>`).join('');
  if ([...select.options].some(o => o.value === current)) select.value = current;
}
function statusBadge(task) {
  return task.status === 'COMPLETED'
    ? '<span class="badge ok">Завершено</span>'
    : '<span class="badge warn">Нет RUN_COMPLETED</span>';
}
function linkageBadge(task) {
  return task.assigned_motor_id
    ? `<span class="badge linked">Motor #${Number(task.assigned_motor_id)}</span>`
    : '<span class="badge">Не привязано</span>';
}
function plannedRepeats(task) {
  const value = Number(task.repeat_target);
  return Number.isInteger(value) && value > 0 ? value : '—';
}
function historicalRuns(task) {
  const key = normalizedProgram(task.program);
  return historicalCounts.has(key) ? historicalCounts.get(key) : '—';
}
function detailText(task) {
  if (task.status !== 'COMPLETED') return 'RUN_STARTED получен, RUN_COMPLETED для exact run_id отсутствует. Такая запись не может считаться выполненной.';
  if (task.start_observed === false) return 'RUN_COMPLETED восстановлен из Arduino EEPROM после потери связи. ESP32 не наблюдала RUN_STARTED, но completion evidence сохранено.';
  return 'RUN_STARTED и RUN_COMPLETED наблюдались для exact session_id + run_id.';
}
function assignedMotorText(task) {
  if (!task.assigned_motor_id) return '—';
  const motor = motors.find(m => Number(m.motor_id) === Number(task.assigned_motor_id));
  return motor ? motorLabel(motor) : `Двигатель №${Number(task.assigned_motor_id)}`;
}
function renderDesktop() {
  const rows = tasks.map(task => {
    const key = taskKey(task);
    const canSelect = task.status === 'COMPLETED';
    return `<tr>
<td><input class="task-select" type="checkbox" data-key="${esc(key)}" ${selected.has(key)?'checked':''} ${canSelect?'':'disabled'} aria-label="Выбрать run ${Number(task.run_id)}"></td>
<td class="count"><b>${Number(task.session_id)}</b> / ${Number(task.run_id)}</td>
<td class="program">${esc(task.program)}</td>
<td class="count">${plannedRepeats(task)}</td>
<td class="count">${Number(task.completed_runs || 0)}</td>
<td class="count">${historicalRuns(task)}</td>
<td>${statusBadge(task)} ${linkageBadge(task)}</td>
<td>${esc(assignedMotorText(task))}</td>
<td><span class="tip" tabindex="0" aria-label="Подробности">i<span class="tip-box">${esc(detailText(task))}<br>Тип: ${esc(task.winding_type || '—')}<br>Роль: ${esc(task.assignment_role || '—')}</span></span></td>
</tr>`;
  }).join('');
  $('items').innerHTML = `<table><thead><tr><th></th><th>Session / Run</th><th>Программа</th><th>План повторов</th><th>Факт. повторов</th><th>Историч. RUN</th><th>Статус</th><th>Двигатель</th><th>Info</th></tr></thead><tbody>${rows || '<tr><td colspan="9">Записей нет.</td></tr>'}</tbody></table>`;
}
function renderMobile() {
  $('items').innerHTML = tasks.map(task => {
    const key = taskKey(task);
    const canSelect = task.status === 'COMPLETED';
    return `<article class="task">
<div class="task-head"><input class="task-select" type="checkbox" data-key="${esc(key)}" ${selected.has(key)?'checked':''} ${canSelect?'':'disabled'} aria-label="Выбрать run ${Number(task.run_id)}"><div><div class="program">${esc(task.program)}</div><div>${statusBadge(task)} ${linkageBadge(task)}</div></div><b>#${Number(task.run_id)}</b></div>
<div class="row"><span class="muted">Session / Run</span><b>${Number(task.session_id)} / ${Number(task.run_id)}</b></div>
<div class="row"><span class="muted">План повторов</span><b>${plannedRepeats(task)}</b></div>
<div class="row"><span class="muted">Факт. повторов</span><b>${Number(task.completed_runs || 0)}</b></div>
<div class="row"><span class="muted">Историч. RUN программы</span><b>${historicalRuns(task)}</b></div>
<div class="row"><span class="muted">Двигатель</span><b>${esc(assignedMotorText(task))}</b></div>
<details><summary>Подробности</summary><p class="mini">${esc(detailText(task))}</p><p class="mini">Тип: ${esc(task.winding_type || '—')} · Роль: ${esc(task.assignment_role || '—')}</p></details>
</article>`;
  }).join('') || '<div class="note">Записей нет.</div>';
}
function bindSelections() {
  document.querySelectorAll('.task-select').forEach(input => {
    input.addEventListener('change', () => {
      if (input.checked) selected.add(input.dataset.key);
      else selected.delete(input.dataset.key);
      updateBulkControls();
    });
  });
}
function renderTasks() {
  if (view === 'desktop') renderDesktop(); else renderMobile();
  bindSelections();
  $('more').hidden = !hasMore;
  updateBulkControls();
}
function selectedTasks() {
  return tasks.filter(task => selected.has(taskKey(task)));
}
function updateBulkControls() {
  const chosen = selectedTasks();
  const completed = chosen.filter(t => t.status === 'COMPLETED');
  $('selectedCount').textContent = `(${chosen.length}${view==='desktop'?' выбрано':''})`;
  const valid = chosen.length > 0 && completed.length === chosen.length;
  $('bulkAssign').disabled = !(valid && $('motorSelect').value);
  $('createSelected').disabled = !valid;
  $('combineSelected').disabled = !(valid && chosen.length > 1);
}
async function assignOne(task, motorId, role) {
  if (task.status !== 'COMPLETED') throw new Error('incomplete_run_cannot_be_assigned');
  const body = new URLSearchParams({
    session_id:String(task.session_id),
    run_id:String(task.run_id),
    motor_id:String(motorId),
    role,
    confirmed:'true'
  });
  return jsonFetch('/api/autonomous-windings/assign', {
    method:'POST',
    headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body
  });
}
async function bulkAssign() {
  const chosen = selectedTasks();
  const motorId = Number($('motorSelect').value);
  const role = $('role').value;
  if (!chosen.length || !motorId) return;
  setBulkState(`Привязка ${chosen.length} записей…`, 'note');
  try {
    for (const task of chosen) await assignOne(task, motorId, role);
    selected.clear();
    await load(true);
    setBulkState(`Привязано записей: ${chosen.length}. Физические RUN events не изменялись.`, 'note ok');
  } catch (error) {
    setBulkState('Ошибка linkage: ' + error.message, 'note warn');
  }
}
function selectedPrograms(chosen) {
  return [...new Set(chosen.map(t => normalizedProgram(t.program)).filter(Boolean))];
}
function motorCreatePayload(chosen, allowMixed) {
  const programs = selectedPrograms(chosen);
  if (!programs.length) throw new Error('selected_program_missing');
  if (!allowMixed && programs.length !== 1) throw new Error('Для «Создать из выбранных» выберите записи одной программы. Для разных программ используйте «Объединить программы».');
  const manufacturer = $('newMaker').value.trim();
  const model = $('newModel').value.trim();
  if (!manufacturer && !model) throw new Error('Укажите производителя или модель.');
  const repeatTarget = Math.max(1, Math.min(65535, Number($('newRepeat').value) || 1));
  const body = new URLSearchParams({
    name:[manufacturer, model].filter(Boolean).join(' ') || `Arduino ${programs[0]}`,
    manufacturer,
    model,
    coil_program:programs[0],
    repeat_target:String(repeatTarget),
    comment:`Создан из Arduino archive selection. Exact runs: ${chosen.map(t => `${t.session_id}/${t.run_id}`).join(', ')}. Programs: ${programs.join(' + ')}`
  });
  const phases = Number($('newPhases').value);
  const slots = Number($('newSlots').value);
  if (Number.isInteger(phases) && phases > 0) body.set('phase_count', String(phases));
  if (Number.isInteger(slots) && slots > 0) body.set('slot_count', String(slots));
  return {body, programs};
}
async function createAndLink(allowMixed) {
  const chosen = selectedTasks();
  if (!chosen.length) return;
  setBulkState('Создание карточки двигателя…', 'note');
  try {
    const {body, programs} = motorCreatePayload(chosen, allowMixed);
    const created = await jsonFetch('/api/motors', {
      method:'POST',
      headers:{'Content-Type':'application/x-www-form-urlencoded'},
      body
    });
    const motorId = Number(created.motor_id);
    if (!motorId) throw new Error('motor_id_missing');
    for (const task of chosen) await assignOne(task, motorId, $('role').value);
    selected.clear();
    motorsLoaded = false;
    motors = [];
    renderMotorOptions();
    await load(true);
    setBulkState(`${programs.length > 1 ? 'Объединено программ' : 'Создан двигатель'}: motor #${motorId}; linked RUN: ${chosen.length}.`, 'note ok');
  } catch (error) {
    setBulkState('Ошибка: ' + error.message, 'note warn');
  }
}
function setBulkState(text, className) {
  $('bulkState').className = className || 'muted';
  $('bulkState').textContent = text;
}
async function load(reset) {
  if (loading) return;
  loading = true;
  try {
    const program = $('program').value.trim();
    const tolerance = clampTolerance();
    if (reset) {
      tasks = [];
      selected.clear();
      nextCursor = null;
      hasMore = false;
      $('items').innerHTML = '<div class="note">Загрузка…</div>';
    }
    $('state').className = 'note';
    $('state').textContent = 'Загрузка bounded страницы архива…';
    const page = await fetchPage(reset ? 0 : nextCursor, program, tolerance, PAGE);
    if (!reset && page.cursor !== nextCursor) throw new Error('cursor_mismatch');
    tasks.push(...(page.items || []));
    hasMore = page.has_more === true;
    nextCursor = hasMore ? Number(page.next_cursor) : null;
    if (hasMore && !Number.isInteger(nextCursor)) throw new Error('invalid_archive_cursor');
    const completed = tasks.filter(t => t.status === 'COMPLETED').length;
    $('state').className = 'note ok';
    $('state').textContent = `${program ? `Программа ${program} ±${tolerance}% · ` : ''}загружено ${tasks.length}; завершено ${completed}; незавершено ${tasks.length-completed}${hasMore?' · есть ещё':' · конец выборки'}.`;
    renderTasks();
  } catch (error) {
    $('state').className = 'note warn';
    $('state').textContent = 'Ошибка: ' + error.message;
    $('more').hidden = true;
  } finally {
    loading = false;
  }
}
async function scanHistory() {
  if (historyLoading) return;
  historyLoading = true;
  $('historyScan').disabled = true;
  setBulkState('Полный read-only scan архива для исторических счётчиков…', 'note');
  try {
    const counts = new Map();
    let cursor = 0;
    let pages = 0;
    for (;;) {
      const page = await fetchPage(cursor, '', 20, ARCHIVE_PAGE);
      for (const task of page.items || []) {
        if (task.status !== 'COMPLETED') continue;
        const key = normalizedProgram(task.program);
        counts.set(key, (counts.get(key) || 0) + 1);
      }
      if (page.has_more !== true) break;
      const next = Number(page.next_cursor);
      if (!Number.isInteger(next) || next <= cursor) throw new Error('invalid_archive_cursor');
      cursor = next;
      if (++pages > 10000) throw new Error('archive_page_limit');
    }
    historicalCounts.clear();
    counts.forEach((value, key) => historicalCounts.set(key, value));
    renderTasks();
    setBulkState(`Исторические счётчики рассчитаны для ${counts.size} программ. Archive не изменялся.`, 'note ok');
  } catch (error) {
    setBulkState('Ошибка подсчёта истории: ' + error.message, 'note warn');
  } finally {
    historyLoading = false;
    $('historyScan').disabled = false;
  }
}

$('search').addEventListener('click', () => load(true));
$('all').addEventListener('click', () => { $('program').value=''; load(true); });
$('more').addEventListener('click', () => load(false));
$('motorSelect').addEventListener('focus', () => { void loadMotors(); });
$('motorSelect').addEventListener('change', updateBulkControls);
$('bulkAssign').addEventListener('click', bulkAssign);
$('createSelected').addEventListener('click', () => createAndLink(false));
$('combineSelected').addEventListener('click', () => createAndLink(true));
$('selectCompleted').addEventListener('click', () => {
  tasks.filter(t => t.status === 'COMPLETED').forEach(t => selected.add(taskKey(t)));
  renderTasks();
});
$('clearSelection').addEventListener('click', () => { selected.clear(); renderTasks(); });
$('historyScan').addEventListener('click', scanHistory);

renderMotorOptions();
load(true);
})();
