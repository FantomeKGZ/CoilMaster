(() => {
'use strict';

const params = new URLSearchParams(location.search);
const motorId = params.get('motor_id') || '';
const roles = document.getElementById('roles');
if (!roles || !/^[1-9][0-9]*$/.test(motorId)) return;

const roleCards = roles.querySelectorAll('.role');
if (roleCards.length < 2) return;

const state = document.createElement('div');
state.id = 'roleSendState';
state.className = 'muted';
state.style.marginTop = '10px';
state.textContent = 'Прямая отправка создаёт сервисное задание без ремонта. Физический START остаётся обязательным.';
roles.insertAdjacentElement('afterend', state);

const buttons = new Map();
let busy = false;

const canonical = value => /^[1-9][0-9]*$/.test(String(value));
const validRepeat = value => canonical(value) && Number(value) <= 65535;

function setState(text, kind) {
  state.className = kind || 'muted';
  state.textContent = text;
}

function setBusy(value) {
  busy = value;
  buttons.forEach(button => { button.disabled = value; });
}

function addButton(card, role) {
  const actions = document.createElement('div');
  actions.className = 'actions';
  actions.style.marginTop = '12px';
  const button = document.createElement('button');
  button.type = 'button';
  button.className = 'primary';
  button.textContent = 'Отправить на станок';
  button.dataset.windingRole = role;
  button.addEventListener('click', () => { void sendRole(role); });
  actions.appendChild(button);
  card.appendChild(actions);
  buttons.set(role, button);
}

addButton(roleCards[0], 'working');
addButton(roleCards[1], 'starting');

async function jsonFetch(url, options) {
  const response = await fetch(url, options || {cache:'no-store'});
  let body = {};
  try { body = await response.json(); } catch (_) {}
  if (!response.ok) throw new Error(body.error || `HTTP_${response.status}`);
  return body;
}

async function authoritativeRole(role) {
  const body = await jsonFetch('/api/motors/winding/latest?motor_id=' + encodeURIComponent(motorId));
  if (body.versioned === true && body.item) {
    if (role === 'starting') {
      if (body.item.starting_present !== true) return null;
      const program = String(body.item.starting_program || '').trim();
      const repeat = body.item.starting_repeat_target;
      return program && validRepeat(repeat) ? {program, repeat:Number(repeat)} : null;
    }
    const program = String(body.item.working_program || '').trim();
    const repeat = body.item.working_repeat_target;
    return program && validRepeat(repeat) ? {program, repeat:Number(repeat)} : null;
  }

  if (role === 'starting') return null;
  const motorBody = await jsonFetch('/api/motors/by-id?motor_id=' + encodeURIComponent(motorId));
  const motor = motorBody.item;
  if (!motor) return null;
  const program = String(motor.coil_program || '').trim();
  const repeat = validRepeat(motor.repeat_target) ? Number(motor.repeat_target) : 1;
  return program ? {program, repeat} : null;
}

async function serviceReady() {
  const status = await jsonFetch('/api/status');
  if (status.job_creation_ready === true) return true;
  if (status.autonomous_run_active === true) throw new Error('arduino_local_winding_active');
  if (status.manual_review_required === true) throw new Error('manual_review_required');
  if (status.new_job_allowed !== true) throw new Error('current_job_not_complete');
  throw new Error('job_creation_not_ready');
}

function responseMatches(body, expectedRepeat) {
  return body && body.accepted === true && body.linked === false &&
         body.repair_id === null && body.motor_id === null && body.spool_id === null &&
         body.spool_selection_saved === false && body.automatic_wire_writeoff_allowed === false &&
         canonical(body.job_id) && canonical(body.session_id) &&
         Number(body.repeat_target) === Number(expectedRepeat);
}

async function sendRole(role) {
  if (busy) return;
  setBusy(true);
  setState('Проверка актуальной обмотки и готовности станка…', 'muted');
  try {
    const selected = await authoritativeRole(role);
    if (!selected) {
      setState(role === 'starting'
        ? 'Пусковая обмотка не зарегистрирована. Отправка заблокирована.'
        : 'Рабочая обмотка недоступна. Отправка заблокирована.', 'warn');
      return;
    }

    await serviceReady();
    const form = new FormData();
    form.set('type', role);
    form.set('turns', selected.program);
    form.set('repeat_target', String(selected.repeat));

    setState('Отправка сохранённой ' + (role === 'starting' ? 'пусковой' : 'рабочей') + ' обмотки на Arduino…', 'muted');
    const body = await jsonFetch('/api/jobs', {method:'POST', body:form});
    if (!responseMatches(body, selected.repeat)) {
      setState('ESP32 приняла запрос, но ответ не подтвердил безопасное сервисное задание. Не запускайте намотку до проверки состояния.', 'warn');
      return;
    }

    setState('Задание №' + body.job_id + ' передано без ремонта. Программа: ' + selected.program + ' × ' + selected.repeat + '. Запуск — только физической кнопкой START.', 'ok');
  } catch (error) {
    setState('Не удалось отправить на станок: ' + (error.message || 'unknown'), 'warn');
  } finally {
    setBusy(false);
  }
}
})();
