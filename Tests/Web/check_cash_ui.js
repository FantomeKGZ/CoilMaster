const fs = require('fs');
const path = require('path');

const root = path.resolve(__dirname, '../..');
const cashPath = 'firmware/esp32/web/desktop/cash.html';
const costingPath = 'firmware/esp32/web/desktop/costing.html';
const cash = fs.readFileSync(path.join(root, cashPath), 'utf8');
const costing = fs.readFileSync(path.join(root, costingPath), 'utf8');
const failures = [];
const must = (token, label) => { if (!cash.includes(token)) failures.push(`${cashPath}: missing ${label}: ${token}`); };
const forbid = (token, label) => { if (cash.includes(token)) failures.push(`${cashPath}: forbidden ${label}: ${token}`); };

for (const [token, label] of [
  ["fetch('/api/repairs?'+q", 'bounded repair list'],
  ["limit:'20'", 'bounded paging'],
  ['/api/payments/balance?repair_id=', 'authoritative repair balance'],
  ["fetch('/api/payments?'+q", 'bounded payment history'],
  ["fetch('/api/payments',{method:'POST'", 'append payment write'],
  ["fd.set('confirmed','true')", 'explicit backend confirmation'],
  ["fd.set('repair_id',String(selectedRepair))", 'exact repair identity'],
  ["fd.set('amount_minor',amountMinor)", 'minor-unit amount'],
  ["fd.set('currency',selectedBalance.currency)", 'authoritative balance currency'],
  ["value=\"PAYMENT\"", 'PAYMENT event'],
  ["value=\"CORRECTION\"", 'CORRECTION event'],
  ["value=\"ADD\"", 'ADD direction'],
  ["value=\"SUBTRACT\"", 'SUBTRACT direction'],
  ["fd.set('corrects_event_id',corrects)", 'correction provenance'],
  ['function decimalToMinor', 'major-to-minor parser'],
  ['BigInt(whole)*100n', 'integer input money conversion'],
  ['const n=BigInt(String(minor??0))', 'integer-exact money rendering'],
  ['debt=b?BigInt(String(b.debt_minor||0))', 'integer-exact debt comparison'],
  ['credit=b?BigInt(String(b.credit_minor||0))', 'integer-exact credit comparison'],
  ['if(!confirm(text))return', 'operator confirmation'],
  ['Старые cash events не редактируются и не удаляются', 'append-only wording'],
  ['Payment разрешён и после CLOSED/DELIVERED', 'post-close payment contract'],
  ['/api/repairs/delivery?repair_id=', 'read-only delivery status'],
  ['/desktop/costing.html?repair_id=', 'separate costing navigation'],
  ['Платёж не управляет станком, SSR или складом', 'machine/warehouse separation']
]) must(token, label);

for (const [token, label] of [
  ["method:'DELETE'", 'destructive payment delete'],
  ["method:'PUT'", 'destructive payment replace'],
  ["method:'PATCH'", 'destructive payment patch'],
  ['/api/material-requests/warehouse', 'warehouse mutation'],
  ['/api/jobs', 'job mutation'],
  ['/api/hardware', 'hardware mutation'],
  ['/api/start', 'start mutation'],
  ['const n=Number(minor)', 'lossy money rendering']
]) forbid(token, label);

if (costing.includes("fetch('/api/payments',{method:'POST'")) {
  failures.push(`${costingPath}: cash mutation must stay out of costing UI`);
}
if (costing.includes('<h1>Касса</h1>')) {
  failures.push(`${costingPath}: costing must remain costing, not cash UI`);
}

if (failures.length) {
  console.error(failures.join('\n'));
  process.exit(1);
}
console.log('Cash UI contracts OK: exact uint64 minor-unit display, exact repair balance, bounded append-only payment history, explicit PAYMENT/CORRECTION writes, no destructive edits, and no machine/warehouse shortcuts.');
