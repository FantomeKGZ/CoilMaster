(() => {
'use strict';
const originalFetch = window.fetch.bind(window);
const completedCacheKey = 'cm-last-completed-arduino-job-v1';

function positiveInt(value) {
  const number = Number(value);
  return Number.isInteger(number) && number > 0 ? number : 0;
}

function snapshot(body) {
  if (!body || !positiveInt(body.job_id)) return null;
  return {
    job_id:positiveInt(body.job_id),
    session_id:positiveInt(body.session_id),
    run_id:positiveInt(body.last_run_id),
    type:String(body.job_type || 'WORKING').toUpperCase(),
    program:String(body.program || ''),
    repeat_target:positiveInt(body.repeat_target) || 1,
    completed_runs:Math.max(0, Number(body.completed_runs) || 0),
    linked:body.linked === true,
    repair_id:positiveInt(body.repair_id),
    motor_id:positiveInt(body.motor_id)
  };
}

function saveCompleted(value) {
  if (!value) return;
  try { localStorage.setItem(completedCacheKey, JSON.stringify(value)); } catch (_) {}
}

function loadCompleted() {
  try {
    const value = JSON.parse(localStorage.getItem(completedCacheKey) || 'null');
    return value && positiveInt(value.job_id) ? value : null;
  } catch (_) { return null; }
}

function labelType(value) {
  return String(value || '').toUpperCase() === 'STARTING' ? 'Пусковая' : 'Рабочая';
}

function appendRow(parent, label, value) {
  const row = document.createElement('div');
  row.className = 'row';
  const left = document.createElement('span');
  left.className = 'muted';
  left.textContent = label;
  const right = document.createElement('b');
  right.textContent = value;
  row.append(left, right);
  parent.appendChild(row);
}

function renderSnapshot(parent, value, completed) {
  if (!parent) return;
  parent.replaceChildren();
  if (!value) {
    const empty = document.createElement('div');
    empty.className = 'muted';
    empty.textContent = completed
      ? 'Завершённое Web-задание ещё не зафиксировано в этом браузере.'
      : 'Задание ещё не отправлялось или данные недоступны.';
    parent.appendChild(empty);
    return;
  }
  appendRow(parent, 'Задание', `№${value.job_id}`);
  appendRow(parent, 'Тип', labelType(value.type));
  appendRow(parent, 'Программа', String(value.program || '').trim() || '—');
  appendRow(parent, 'Повторы', completed
    ? `${value.completed_runs} / ${value.repeat_target}`
    : String(value.repeat_target));
  appendRow(parent, 'Контекст', value.linked
    ? `Ремонт №${value.repair_id} · двигатель №${value.motor_id}`
    : 'Без привязки');
  if (completed) appendRow(parent, 'Последний RUN', value.run_id ? `№${value.run_id}` : '—');
}

function ensureDashboardUi() {
  if (document.getElementById('cmLastCompletedArduinoJob')) return;
  const main = document.querySelector('main');
  if (!main) return;
  const mobile = location.pathname.startsWith('/mobile/');
  const heading = [...main.querySelectorAll('h2,h3')]
    .find(node => node.textContent.trim() === 'Текущее задание' || node.textContent.trim() === 'Намотка');
  if (!heading) return;

  heading.textContent = 'Последнее отправленное на Arduino';
  const sent = document.createElement('div');
  sent.id = 'cmLastSentArduinoJob';
  sent.style.marginBottom = '10px';
  heading.insertAdjacentElement('afterend', sent);

  const section = heading.closest('section');
  if (!section || !section.parentNode) return;
  const completedSection = document.createElement('section');
  completedSection.className = 'card';
  const completedHeading = document.createElement(mobile ? 'h2' : 'h3');
  completedHeading.textContent = 'Последнее выполненное на Arduino';
  const completedBox = document.createElement('div');
  completedBox.id = 'cmLastCompletedArduinoJob';
  const note = document.createElement('div');
  note.className = 'muted';
  note.style.marginTop = '8px';
  note.textContent = 'Информационный browser-local снимок; authoritative RUN evidence остаётся в журналах CoilMaster и Arduino-архиве.';
  completedSection.append(completedHeading, completedBox, note);
  section.insertAdjacentElement('afterend', completedSection);

  renderSnapshot(sent, loadCompleted(), false);
  renderSnapshot(completedBox, loadCompleted(), true);
}

function updateDashboard(raw) {
  ensureDashboardUi();
  const current = snapshot(raw);
  const target = Number(raw && raw.repeat_target);
  const completed = Number(raw && raw.completed_runs);
  if (raw && raw.job_status === 'PROGRAM_COMPLETED' && current &&
      Number.isFinite(target) && target > 0 &&
      Number.isFinite(completed) && completed >= target) {
    saveCompleted(current);
  }
  renderSnapshot(document.getElementById('cmLastSentArduinoJob'), current || loadCompleted(), false);
  renderSnapshot(document.getElementById('cmLastCompletedArduinoJob'), loadCompleted(), true);
}

window.fetch = async function(input, init) {
  const requestUrl = typeof input === 'string' ? input : input && input.url;
  const method = String((init && init.method) || (input && input.method) || 'GET').toUpperCase();
  if (!requestUrl || method !== 'GET') return originalFetch(input, init);

  const url = new URL(requestUrl, location.origin);
  if (url.pathname !== '/api/status') return originalFetch(input, init);

  const response = await originalFetch(input, init);
  if (!response.ok) return response;

  let body;
  try { body = await response.clone().json(); }
  catch (_) { return response; }

  updateDashboard(body);

  const target = Number(body.repeat_target);
  const completed = Number(body.completed_runs);
  if (body.job_status !== 'PROGRAM_COMPLETED' ||
      !Number.isFinite(target) || target <= 0 ||
      !Number.isFinite(completed) || completed < target) {
    return response;
  }

  const completedSnapshot = snapshot(body);
  const display = {
    ...body,
    last_completed_job_id:body.job_id,
    last_completed_session_id:body.session_id,
    last_completed_run_id:body.last_run_id,
    last_completed_job_type:completedSnapshot ? completedSnapshot.type : body.job_type,
    last_completed_program:completedSnapshot ? completedSnapshot.program : body.program,
    last_completed_repeat_target:completedSnapshot ? completedSnapshot.repeat_target : body.repeat_target,
    last_completed_runs:completedSnapshot ? completedSnapshot.completed_runs : body.completed_runs,
    current_job_cleared_after_completion:true,
    job_id:0,
    session_id:0,
    job_status:'IDLE',
    machine_status:'Ожидание',
    job_type:'WORKING',
    program:'',
    repeat_target:1,
    completed_runs:0,
    linked:false,
    repair_id:null,
    motor_id:null,
    spool_id:null,
    spool_wire_type:null,
    spool_diameter_hundredths_mm:null,
    spool_weight_at_selection_g:null,
    run_active:false,
    arduino_ack_pending:false,
    arduino_cancel_pending:false
  };
  return new Response(JSON.stringify(display), {
    status:response.status,
    statusText:response.statusText,
    headers:{'Content-Type':'application/json; charset=utf-8'}
  });
};

document.addEventListener('DOMContentLoaded', ensureDashboardUi, {once:true});
})();
