const fs = require('fs');
const path = require('path');

const root = path.resolve(__dirname, '../..');
const web = path.join(root, 'firmware/esp32/web');
const failures = [];
const read = rel => fs.readFileSync(path.join(web, rel), 'utf8');
const need = (source, token, label) => { if (!source.includes(token)) failures.push(`${label}: missing ${token}`); };
const forbid = (source, token, label) => { if (source.includes(token)) failures.push(`${label}: embedded CRUD token ${token}`); };

function compileScripts(rel) {
  const source = read(rel);
  const scripts = [...source.matchAll(/<script[^>]*>([\s\S]*?)<\/script>/gi)].map(m => m[1]);
  scripts.forEach((script, index) => {
    try { new Function(script); }
    catch (error) { failures.push(`${rel}: script ${index + 1} syntax error: ${error.message}`); }
  });
}

for (const surface of ['desktop', 'mobile']) {
  const repairs = read(`${surface}/repairs.html`);
  need(repairs, `/${surface}/repair-new.html`, `${surface} repairs new repair link`);
  need(repairs, `/${surface}/client-new.html`, `${surface} repairs new client link`);
  need(repairs, `/${surface}/motor-new.html`, `${surface} repairs new motor link`);
  forbid(repairs, "fetch('/api/repairs',{method:'POST'", `${surface} repairs list`);
  forbid(repairs, 'name="received_at"', `${surface} repairs list`);

  const repairNew = read(`${surface}/repair-new.html`);
  need(repairNew, "fetch('/api/repairs'", `${surface} repair creation`);
  need(repairNew, 'name="client_id"', `${surface} repair client link`);
  need(repairNew, 'name="motor_id"', `${surface} repair motor link`);
  need(repairNew, 'let sending=false', `${surface} repair create state guard`);
  need(repairNew, 'if(sending)return', `${surface} repair create single-flight`);
  need(repairNew, "$('saveBtn').disabled=true", `${surface} repair submit lock`);
  need(repairNew, 'repair_id_missing', `${surface} repair create response identity`);

  const clients = read(`${surface}/clients.html`);
  need(clients, `/${surface}/client-new.html`, `${surface} client catalog create link`);
  forbid(clients, "fetch('/api/clients',{method:'POST'", `${surface} client catalog`);

  const clientNew = read(`${surface}/client-new.html`);
  need(clientNew, "fetch('/api/clients'", `${surface} client creation`);
  need(clientNew, 'if(sending)return', `${surface} client create single-flight`);

  const motors = read(`${surface}/motors.html`);
  need(motors, `/${surface}/motor-new.html`, `${surface} motor catalog create link`);
  need(motors, `/${surface}/repair-new.html`, `${surface} motor catalog repair link`);
  forbid(motors, "fetch('/api/motors',{method:'POST'", `${surface} motor catalog`);

  const motorNew = read(`${surface}/motor-new.html`);
  need(motorNew, "fetch('/api/motors'", `${surface} motor creation`);
  need(motorNew, 'checking=false,creating=false', `${surface} motor create state guards`);
  need(motorNew, 'if(creating)return', `${surface} motor create single-flight`);
  need(motorNew, 'if(checking||creating)return', `${surface} motor similarity single-flight`);

  const motorEdit = read(`${surface}/motor-edit.html`);
  need(motorEdit, '/api/motors/update', `${surface} motor editing`);

  const warehouse = read(`${surface}/warehouse.html`);
  need(warehouse, `/${surface}/spool-new.html`, `${surface} warehouse spool create link`);
  forbid(warehouse, 'id="spoolForm"', `${surface} warehouse list`);
  forbid(warehouse, "fetch('/api/warehouse/spools',{method:'POST'", `${surface} warehouse list`);
  need(warehouse, '/api/warehouse/spools/material', `${surface} legacy spool classification retained`);

  const spoolNew = read(`${surface}/spool-new.html`);
  need(spoolNew, "fetch('/api/warehouse/spools'", `${surface} spool creation`);
  need(spoolNew, 'let sending=false', `${surface} spool create state guard`);
  need(spoolNew, 'if(sending)return', `${surface} spool create single-flight`);
  need(spoolNew, "$('saveBtn').disabled=true", `${surface} spool submit lock`);
  need(spoolNew, 'spool_id_missing', `${surface} spool create response identity`);

  const materials = read(`${surface}/material-catalog.html`);
  need(materials, `/${surface}/material-new.html`, `${surface} material create link`);
  need(materials, `/${surface}/material-edit.html?material_id=`, `${surface} material edit link`);
  forbid(materials, "fetch('/api/materials',{method:'POST'", `${surface} material catalog`);
  forbid(materials, "fetch('/api/materials/adjust'", `${surface} material catalog inline edit`);

  const materialNew = read(`${surface}/material-new.html`);
  need(materialNew, "fetch('/api/materials'", `${surface} material creation`);
  need(materialNew, 'let sending=false', `${surface} material create state guard`);
  need(materialNew, 'if(sending)return', `${surface} material create single-flight`);
  need(materialNew, "$('saveBtn').disabled=true", `${surface} material submit lock`);
  need(materialNew, 'material_id_missing', `${surface} material create response identity`);
  const materialEdit = read(`${surface}/material-edit.html`);
  need(materialEdit, "fetch('/api/materials/adjust'", `${surface} material editing`);

  for (const rel of [
    `${surface}/repairs.html`, `${surface}/repair-new.html`,
    `${surface}/clients.html`, `${surface}/client-new.html`,
    `${surface}/motors.html`, `${surface}/motor-new.html`, `${surface}/motor-edit.html`,
    `${surface}/warehouse.html`, `${surface}/spool-new.html`,
    `${surface}/material-catalog.html`, `${surface}/material-new.html`, `${surface}/material-edit.html`
  ]) compileScripts(rel);
}

if (failures.length) {
  console.error(failures.join('\n'));
  process.exit(1);
}
console.log('CRUD page separation OK: catalogs are list-only, create/edit flows are dedicated pages, client/motor/repair/spool/material create mutations are single-flight, navigation is present, and modified scripts parse.');
