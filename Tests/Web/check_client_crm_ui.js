const fs = require('fs');
const path = require('path');

const root = path.resolve(__dirname, '../..');
const failures = [];
const read = p => fs.readFileSync(path.join(root, p), 'utf8');
const must = (src, token, label) => { if (!src.includes(token)) failures.push(`${label}: missing ${token}`); };
const mustNot = (src, token, label) => { if (src.includes(token)) failures.push(`${label}: forbidden ${token}`); };

const clientsPath = 'firmware/esp32/web/desktop/clients.html';
const newPath = 'firmware/esp32/web/desktop/client-new.html';
const detailsPath = 'firmware/esp32/web/desktop/client-details.html';
const repairsPath = 'firmware/esp32/web/desktop/repairs.html';

const clients = read(clientsPath);
const clientNew = read(newPath);
const details = read(detailsPath);
const repairs = read(repairsPath);

must(clients, '/desktop/client-new.html', 'clients catalog');
must(clients, '/desktop/client-details.html?client_id=', 'clients catalog');
must(clients, "fetch('/api/clients?'+q", 'clients bounded API');
must(clients, "limit:'20'", 'clients page bound');
mustNot(clients, "method:'POST'", 'clients catalog must be read-only');
mustNot(clients, '<form id="form">', 'clients catalog must not contain create form');

must(clientNew, "fetch('/api/clients',{method:'POST'", 'dedicated client creation');
must(clientNew, '/desktop/client-details.html?client_id=', 'new client details handoff');
must(clientNew, '/desktop/repairs.html?client_id=', 'new client repair handoff');
must(clientNew, 'Связь клиента с двигателем фиксируется исторически через ремонт', 'motor ownership boundary');
mustNot(clientNew, "fetch('/api/repairs',{method:'POST'", 'client create must not create repair');

for (const token of [
  '/api/clients/by-id?client_id=',
  '/api/payments/balance?client_id=',
  "fetch('/api/payments?'+q",
  "fetch('/api/repairs?'+q",
  '/api/repairs/delivery?repair_id=',
  '/api/motors/by-id?motor_id=',
  'charged_minor',
  'paid_minor',
  'debt_minor',
  'credit_minor',
  'x.kind||x.event_type||x.type',
  'const item=j.item||j.delivery||j',
  "limit:'12'",
  "limit:'20'",
  'invalid_repair_cursor',
  'invalid_payment_cursor'
]) must(details, token, 'client details');

must(details, 'Выдача двигателя не зависит от нулевого баланса', 'delivery/cash independence');
mustNot(details, "method:'POST'", 'client details must remain read-only');
mustNot(details, '/api/motors',{ }, '');

must(repairs, '/desktop/client-new.html', 'repair client-create navigation');
mustNot(repairs, 'clientForm', 'repair page duplicate client form');
mustNot(repairs, 'clientResult', 'repair page duplicate client handler');

if (failures.length) {
  console.error(failures.join('\n'));
  process.exit(1);
}
console.log('Client CRM UI contracts OK: catalog-only clients, dedicated creation, bounded read-only client details, historical motor linkage, append-only cash reads, and independent delivery state.');
