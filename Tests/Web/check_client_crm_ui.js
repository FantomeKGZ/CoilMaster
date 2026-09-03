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

for (const [label, source, surface] of [
  ['desktop client details', details, 'desktop'],
  ['mobile client details', mobileDetails, 'mobile']
]) {
  must(source, '/api/clients/by-id?client_id=', `${label} client lookup`);
  must(source, '/api/payments/balance?client_id=', `${label} balance lookup`);
  must(source, "fetch('/api/payments?'+q", `${label} bounded payment history`);
  must(source, "fetch('/api/repairs?'+q", `${label} bounded repair history`);
  must(source, "new URLSearchParams({client_id:String(", `${label} client-scoped paging`);
  must(source, "limit:'12'", `${label} repair page bound`);
  must(source, "limit:'20'", `${label} payment page bound`);
  must(source, 'invalid_repair_cursor', `${label} repair cursor guard`);
  must(source, 'invalid_payment_cursor', `${label} payment cursor guard`);
  mustNot(source, "status:'ALL'", `${label} unsupported ALL repair status`);
  must(source, '/api/motors/by-id?motor_id=', `${label} repair motor lookup`);
  must(source, `/${surface}/motor-details.html?motor_id=`, `${label} motor navigation`);
  must(source, `/${surface}/winding-history.html?repair_id=`, `${label} winding history navigation`);
  must(source, `/${surface}/costing.html?repair_id=`, `${label} costing navigation`);
  must(source, '/api/clients/update', `${label} edit endpoint`);
  must(source, "kind:'PREPAYMENT'", `${label} prepayment kind`);
  must(source, "confirmed:'true'", `${label} explicit prepayment confirmation`);
  must(source, 'Зачислить предоплату', `${label} prepayment action`);
  must(source, 'автоматически не засчитывается', `${label} no automatic repair offset wording`);
  must(source, 'prepayment_minor', `${label} separate prepayment balance`);
  must(source, 'paid_minor', `${label} paid balance`);
  must(source, 'debt_minor', `${label} debt balance`);
  must(source, '/api/repairs/delivery?repair_id=', `${label} delivery state`);
  must(source, 'Закрыт, не выдан', `${label} explicit not-delivered state`);
  must(source, 'Выдан ', `${label} delivered timestamp state`);
  must(source, 'const item=j.item||j.delivery||j', `${label} independent delivery payload`);
}

must(details, 'Баланс ремонтов рассчитывается из authoritative стоимости', 'repair costing remains independent from prepayment');
must(mobileDetails, "delivery=closed?await deliveryLabel(x.repair_id):''", 'mobile CLOSED-only delivery lookup');
must(mobileDetails, "closed?' · '+esc(delivery):''", 'mobile repair delivery rendering');

must(repairs, '/desktop/client-new.html', 'repair client-create navigation');
mustNot(repairs, 'clientForm', 'repair page duplicate client form');
mustNot(repairs, 'clientResult', 'repair page duplicate client handler');

if (failures.length) {
  console.error(failures.join('\n'));
  process.exit(1);
}
console.log('Client CRM UI contracts OK: catalog-only browsing, desktop/mobile dedicated creation/editing, valid bounded repair/payment history, repair navigation and delivery-history parity, separate explicit PREPAYMENT, historical motor linkage, and independent repair costing/delivery state.');
