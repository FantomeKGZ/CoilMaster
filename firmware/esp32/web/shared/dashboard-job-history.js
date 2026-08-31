(() => {
'use strict';

const originalFetch = window.fetch.bind(window);
const cacheKey = 'cm-dashboard-last-completed-job-v1';
let lastRawStatus = null;

function asPositiveInt(value) {
  const number = Number(value);
  return Number.isInteger(number) && number > 0 ? number : 0;
}

function typeLabel(value) {
  return String(value || '').toUpperCase() === 'STARTING' ? 'Пусковая' : 'Рабочая';
}

function programLabel(value) {
  const text = String(value || '').trim();
  return text || '—';
}

function contextLabel(status) {
  if (status && status.linked === true) {
    return `Ремонт №${asPositiveInt(status.repair_id) || '—'} · двигатель №${asPositiveInt(status.motor_id) || '—'}`;
  }
  return 'Без привязки к ремонту';
}

function snapshotFrom(status) {
  if (!status || !asPositiveInt(status.job_id)) return null;
  return {
    job_id:asPositiveInt(status.job_id),
    session_id:asPositiveInt(status.session_id),
    run_id:asPositiveInt(status.last_run_id),
    type:String(status.job_type || 'WORKING').toUpperCase(),
    program:String(status.program || ''),
    repeat_target:asPositiveInt(status.repeat_target) || 1,
    completed_runs:Math.max(0, Number(status.completed_runs) || 0),
    linked:status.linked === true,
    repair_id:asPositiveInt(status.repair_id),
    motor_id:asPositiveInt(status.motor_id)
  };
}

function saveCompleted(snapshot) {
  if (!snapshot) return;
  try { localStorage.setItem(cacheKey, JSON.stringify(snapshot)); } catch (_) {}
}

function loadCompleted() {
  try {
    const parsed = JSON.parse(localStorage.getItem(cacheKey) || 'null');
    return parsed && asPositiveInt(parsed.job_id) ? parsed : null;
  } catch (_) { return null; }
}

function row(label, value) {
  const div = document.createElement('div');
  div.className = 'row';
  const left = document.createElement('span');
  left.className = 'muted';
  left.textContent = label;
  const right = document.createElement('b');
  right.textContent = value;
  div.append(left, right);
  return div;
}

function renderBox(box, snapshot, completed) {
  box.replaceChildren();
  if (!snapshot) {
    const note = document.createElement('div');
    note.className = 'muted';
    note.textContent = completed ? 'Завершённых Web-заданий в этом браузере ещё не зафиксировано.' : 'Задание ещё не отправлялось или данные недоступны.';
    box.appendChild(note);
    return;
  }
  box.appendChild(row('Задание', `№${snapshot.job_id}`));
  box.appendChild(row('Тип', typeLabel(snapshot.type)));
  box.appendChild(row('Программа', programLabel(snapshot.program)));
  box.appendChild(row('Повторы', completed ? `${snapshot.completed_runs} / ${snapshot.repeat_target}` : String(snapshot.repeat_target)));
  box.appendChild(row('Контекст', snapshot.linked ? `Ремонт №${snapshot.repair_id} · двигатель №${snapshot.motor_id}` : 'Без привязки'));
  if (completed) box.appendChild(row('Последний RUN', snapshot.run_id ? `№${snapshot.run_id}` : '—'));
}

function ensureUi() {
  if (document.getElementById('lastCompletedArduinoJob')) return;
  const mobile = location.pathname.startsWith('/mobile/');
  const main = document.querySelector('main');
  if (!main) return;

  const currentHeading = [...main.querySelectorAll('h2,h3')].find(h => h.textContent.trim() === 'Текущее задание' || h.textContent.trim() === 'Намотка');
  if (currentHeading) currentHeading.textContent = 'Последнее отправленное на Arduino';

  const currentSection = currentHeading ? currentHeading.closest('section') : null;
  const completedSection = document.createElement('section');
  completedSection.className = 'card';
  completedSection.innerHTML = `<h${mobile ? '2' : '3'}>Последнее выполненное на Arduino</h${mobile ? '2' : '3'}><div id="lastCompletedArduinoJob"></div><div class="muted" style="margin-top:8px">Информационный browser-local снимок. Authoritative RUN evidence остаётся в журналах CoilMaster и Arduino-архиве.</div>`;
  if (currentSection && currentSection.parentNode) currentSection.insertAdjacentElement('afterend', completedSection);
  else main.appendChild(completedSection);

  const sent = document.createElement('div');
  sent.id = 'lastSentArduinoJob';
  sent.style.marginBottom = '10px';
  if (currentHeading) currentHeading.insertAdjacentElement('afterend', sent);

  renderBox(sent, lastRawStatus ? snapshotFrom(lastRawStatus) : null, false);
  renderBox(completedSection.querySelector('#lastCompletedArduinoJob'), loadCompleted(), true);
}

function updateUi(status) {
  lastRawStatus = status;
  const snapshot = snapshotFrom(status);
  if (status && status.job_status === 'PROGRAM_COMPLETED' && snapshot && snapshot.completed_runs >= snapshot.repeat_target) {
    saveCompleted(snapshot);
  }
  ensureUi();
  const sent = document.getElementById('lastSentArduinoJob');
  const completed = document.getElementById('lastCompletedArduinoJob');
  if (sent) renderBox(sent, snapshot || loadCompleted(), false);
  if (completed) renderBox(completed, loadCompleted(), true);
}

window.fetch = async function(input, init) {
  const requestUrl = typeof input === 'string' ? input : input && input.url;
  const method = String((init && init.method) || (input && input.method) || 'GET').toUpperCase();
  const response = await originalFetch(input, init);
  if (!requestUrl || method !== 'GET' || !response.ok) return response;
  const url = new URL(requestUrl, location.origin);
  if (url.pathname !== '/api/status') return response;
  try {
    const status = await response.clone().json();
    updateUi(status);
  } catch (_) {}
  return response;
};

document.addEventListener('DOMContentLoaded', ensureUi, {once:true});
})();
