'use strict';

const fs = require('fs');

const variants = [
  ['desktop', 'firmware/esp32/web/desktop/repair-new.html', '/desktop/repairs.html'],
  ['mobile', 'firmware/esp32/web/mobile/repair-new.html', '/mobile/repairs.html'],
];

function requireText(source, needle, message) {
  if (!source.includes(needle)) throw new Error(message);
}

function forbidText(source, needle, message) {
  if (source.includes(needle)) throw new Error(message);
}

for (const [variant, path, returnPath] of variants) {
  const source = fs.readFileSync(path, 'utf8');
  for (const token of [
    'name="client_id"',
    'name="motor_id"',
    'name="received_at"',
    'name="complaint"',
    'name="comment"',
    '/api/clients/by-id',
    '/api/motors/by-id',
    "fetch('/api/repairs',{method:'POST',body:new FormData(e.target)})",
    "if(!j.repair_id)throw new Error('repair_id_missing')",
    "localStorage.setItem('cm-active-repair',String(j.repair_id))",
    "let sending=false",
    "if(sending)return",
    "$('saveBtn').disabled=true",
    "finally{sending=false;$('saveBtn').disabled=false}",
  ]) requireText(source, token, `${variant} repair-new contract missing: ${token}`);

  requireText(source, `location.href='${returnPath}'`,
    `${variant} repair-new must return to the matching repairs UI`);
  forbidText(source, '/api/repairs/intake',
    `${variant} repair-new must use canonical POST /api/repairs, not a parallel intake endpoint`);
  forbidText(source, '/api/repairs/create',
    `${variant} repair-new must not reintroduce a legacy create endpoint`);
}

for (const variant of ['desktop', 'mobile']) {
  const path = `firmware/esp32/web/${variant}/repairs.html`;
  const source = fs.readFileSync(path, 'utf8');
  for (const token of [
    "params.get('client_id')",
    "params.get('motor_id')",
    `/${variant}/repair-new.html`,
    "set('client_id'",
    "set('motor_id'",
    'location.replace(',
  ]) requireText(source, token, `${variant} repairs legacy creation handoff missing: ${token}`);
}

const mobileRepairs = fs.readFileSync('firmware/esp32/web/mobile/repairs.html', 'utf8');
forbidText(mobileRepairs, "q.set('client_id',params.get('client_id'))",
  'mobile repairs must not reinterpret the legacy client_id creation handoff as a hidden catalog filter');

console.log('Repair creation UI contracts OK: desktop/mobile use canonical transactional POST /api/repairs with exact client/motor lookup, duplicate-submit guard, and matching legacy client/motor handoff into repair-new.');
