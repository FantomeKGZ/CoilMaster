'use strict';

const fs=require('fs');

const desktop=fs.readFileSync('firmware/esp32/web/desktop/repairs.html','utf8');
const mobile=fs.readFileSync('firmware/esp32/web/mobile/repairs.html','utf8');
const backend=fs.readFileSync('firmware/esp32/src/CM_RepairDeliveryWeb.cpp','utf8');

function must(source,text,label){if(!source.includes(text))throw new Error('Missing '+label+': '+text)}
function forbid(source,text,label){if(source.includes(text))throw new Error('Forbidden '+label+': '+text)}

for(const [label,source] of [['desktop',desktop],['mobile',mobile]]){
  must(source,"fetch('/api/repairs/delivery?repair_id='",label+' delivery read');
  must(source,"fetch('/api/repairs/delivery',{method:'POST'",label+' delivery write');
  must(source,"confirmed:'true'",label+' explicit confirmation payload');
  must(source,"delivered_at:new Date().toISOString()",label+' operator delivery timestamp');
  must(source,"if(r.status===404)",label+' not-delivered state');
  must(source,"closed=status==='CLOSED'",label+' CLOSED state ownership');
  must(source,"else{const box=document.createElement('div')",label+' CLOSED-only delivery controls');
  must(source,'Отметить выдачу',label+' operator action');
  must(source,'Выдача записывается один раз и не зависит от баланса.',label+' immutable/balance-independent confirmation');
  must(source,'Баланс не блокирует выдачу.',label+' balance-independent operator wording');
  forbid(source,"fetch('/api/payments/balance",label+' delivery must not acquire cash balance gate');
  forbid(source,"method:'DELETE'",label+' destructive delivery mutation');
  forbid(source,"method:'PUT'",label+' mutable delivery mutation');
  forbid(source,"method:'PATCH'",label+' mutable delivery mutation');
}

must(backend,'m_server.on("/api/repairs/delivery", HTTP_POST','delivery POST route');
must(backend,'m_server.arg("confirmed") != "true"','backend explicit confirmation gate');
must(backend,'if (repairOpen)','backend CLOSED-only gate');
must(backend,'repair_must_be_closed_before_delivery','backend CLOSED-only error');
must(backend,'repair_already_delivered','backend append-once error');
must(backend,'balance_gate_applied\\\":false','backend balance-independent evidence');
forbid(backend,'/api/payments','delivery backend cash coupling');

console.log('Repair delivery UI contracts OK: CLOSED-only, explicitly confirmed, append-once and balance-independent.');
