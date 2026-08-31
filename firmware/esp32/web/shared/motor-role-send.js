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
state.textContent = 'Для отправки на станок будет выбран только однозначный открытый ремонт этого двигателя.';
roles.insertAdjacentElement('afterend', state);

const buttons = new Map();
let busy = false;

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
  button.addEventListener('click', () => { void openRole(role); });
  actions.appendChild(button);
  card.appendChild(actions);
  buttons.set(role, button);
}

addButton(roleCards[0], 'working');
addButton(roleCards[1], 'starting');

async function jsonFetch(url) {
  const response = await fetch(url, {cache:'no-store'});
  let body = {};
  try { body = await response.json(); } catch (_) {}
  if (!response.ok) throw new Error(body.error || `HTTP_${response.status}`);
  return body;
}

async function roleAvailable(role) {
  const body = await jsonFetch('/api/motors/winding/latest?motor_id=' + encodeURIComponent(motorId));
  if (role === 'starting') {
    return body.versioned === true && body.item && body.item.starting_present === true &&
           String(body.item.starting_program || '').trim().length > 0;
  }
  if (body.versioned === true && body.item) {
    return String(body.item.working_program || '').trim().length > 0;
  }
  const program = document.getElementById('workingProgram');
  return program && program.textContent.trim() !== '' && program.textContent.trim() !== '—';
}

async function openRepairs() {
  const result = [];
  let cursor = 0;
  for (let page = 0; page < 100; ++page) {
    const query = new URLSearchParams({motor_id:motorId, limit:'20'});
    if (cursor) query.set('cursor', String(cursor));
    const body = await jsonFetch('/api/motors/repairs?' + query);
    for (const repair of body.items || []) {
      const status = repair.current_status || repair.status || 'OPEN';
      if (status !== 'CLOSED') result.push(repair);
    }
    if (body.has_more !== true) return result;
    const next = Number(body.next_cursor);
    if (!Number.isInteger(next) || next <= cursor) throw new Error('invalid_repair_cursor');
    cursor = next;
  }
  throw new Error('repair_page_limit');
}

function windingJobUrl(repairId, role) {
  const mobile = location.pathname.startsWith('/mobile/');
  const root = mobile ? '/mobile/' : '/desktop/';
  return root + 'winding-job.html?repair_id=' + encodeURIComponent(repairId) + '&role=' + encodeURIComponent(role);
}

async function openRole(role) {
  if (busy) return;
  setBusy(true);
  setState('Проверка роли обмотки и открытого ремонта…', 'muted');
  try {
    if (!await roleAvailable(role)) {
      setState(role === 'starting'
        ? 'Пусковая обмотка не зарегистрирована. Отправка на станок заблокирована.'
        : 'Рабочая обмотка недоступна. Отправка на станок заблокирована.', 'warn');
      return;
    }

    const repairs = await openRepairs();
    if (repairs.length === 0) {
      setState('У двигателя нет открытого ремонта. Сначала создайте или откройте ремонт, затем отправьте обмотку на станок.', 'warn');
      return;
    }
    if (repairs.length > 1) {
      setState('У двигателя несколько открытых ремонтов. Автоматический выбор запрещён — выберите нужный ремонт в списке ниже.', 'warn');
      const repairList = document.getElementById('repairs');
      if (repairList) repairList.scrollIntoView({block:'start'});
      return;
    }

    location.href = windingJobUrl(repairs[0].repair_id, role);
  } catch (error) {
    setState('Не удалось подготовить отправку: ' + (error.message || 'unknown'), 'warn');
  } finally {
    setBusy(false);
  }
}
})();
