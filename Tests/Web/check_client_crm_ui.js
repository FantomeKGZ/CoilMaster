require('./check_cash_ui.js');
const fs = require('fs');
const path = require('path');

const root = path.resolve(__dirname, '../..');
const failures = [];
const read = p => fs.readFileSync(path.join(root, p), 'utf8');
const must = (src, token, label) => { if (!src.includes(token)) failures.push(`${label}: missing ${token}`); };
const mustNot = (src, token, label) => { if (src.includes(token)) failures.push(`${label}: forbidden ${token}`); };

const clientsPath = 'firmware/esp32/web/desktop/clients.html';
const newPath = 'firmware/esp32/web/desktop/client-new.html';
const mobileNewPath = 'firmware/esp32/web/mobile/client-new.html';
const detailsPath = 'firmware/esp32/web/desktop/client-details.html';
const mobileDetailsPath = 'firmware/esp32/web/mobile/client-details.html';
const repairsPath = 'firmware/esp32/web/desktop/repairs.html';

const clients = read(clientsPath);
const clientNew = read(newPath);
const mobileClientNew = read(mobileNewPath);
const details = read(detailsPath);
const mobileDetails = read(mobileDetailsPath);
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

for (const [label, source] of [['desktop client create', clientNew], ['mobile client create', mobileClientNew]]) {
  must(source, 'let sending=false', `${label} single-flight state`);
  must(source, 'if(sending)return', `${label} duplicate submit guard`);
  must(source, "fetch('/api/clients',{method:'POST'", `${label} create endpoint`);
  must(source, 'client_id_missing', `${label} authoritative created identity required`);
  must(source, 'maxlength="96"', `${label} name bound`);
  must(source, 'maxlength="48"', `${label} phone bound`);
  must(source, 'maxlength="320"', `${label} comment bound`);
}
must(clientNew, "$('save').disabled=true", 'desktop client create pending button lock');
must(mobileClientNew, "$('save').disabled=true", 'mobile client create pending button lock');

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
  'prepayment_minor',
  "limit:'12'",
  "limit:'20'",
  'invalid_repair_cursor',
  'invalid_payment_cursor'
]) must(details, token, 'client details');

// Client detail is now the authoritative CRM edit/prepayment surface. The
// catalog remains read-only; writes are explicit, append-only and scoped here.
for (const [label, source] of [['desktop client details', details], ['mobile client details', mobileDetails]]) {
  must(source, '/api/clients/update', `${label} edit endpoint`);
  must(source, "kind:'PREPAYMENT'", `${label} prepayment kind`);
  must(source, "confirmed:'true'", `${label} explicit prepayment confirmation`);
  must(source, 'Зачислить предоплату', `${label} prepayment action`);
  must(source, 'автоматически не засчитывается', `${label} no automatic repair offset wording`);
}

must(details, 'Баланс ремонтов рассчитывается из authoritative стоимости', 'repair costing remains independent from prepayment');
must(details, 'Предоплата', 'separate prepayment balance label');
must(details, 'const item=j.item||j.delivery||j', 'delivery state remains independent and explicit');

must(repairs, '/desktop/client-new.html', 'repair client-create navigation');
mustNot(repairs, 'clientForm', 'repair page duplicate client form');
mustNot(repairs, 'clientResult', 'repair page duplicate client handler');

if (failures.length) {
  console.error(failures.join('\n'));
  process.exit(1);
}
console.log('Client CRM UI contracts OK: catalog-only browsing, desktop/mobile single-flight dedicated creation, append-only client editing, bounded repair/payment history, separate explicit PREPAYMENT, historical motor linkage, and independent repair costing/delivery state.');
