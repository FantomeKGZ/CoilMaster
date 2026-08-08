(()=>{
'use strict';
const p=new URLSearchParams(location.search),repairId=p.get('repair_id')||localStorage.getItem('cm-active-repair')||'';
if(!/^[1-9]\d*$/.test(repairId))return;
const firstCard=document.querySelector('.card');
if(!firstCard)return;

const box=document.createElement('div');
box.className='note muted';
box.style.marginTop='10px';
box.textContent='Проверка immutable бухт завершённых сессий…';
firstCard.appendChild(box);

const esc=v=>String(v??'').replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
const diameter=v=>(Number(v)/100).toFixed(2).replace('.',',');
const kg=v=>(Number(v)/1000).toFixed(3).replace('.',',');

async function loadCompletedSessions(){
    const sessions=[];
    const seen=new Set();
    let cursor=0,pages=0;
    for(;;){
        const r=await fetch('/api/winding-history?repair_id='+encodeURIComponent(repairId)+'&cursor='+cursor+'&limit=100',{cache:'no-store'});
        const j=await r.json();
        if(!r.ok)throw new Error(j.error||'winding_history_read_failed');
        const events=Array.isArray(j.events)?j.events:[];
        for(const e of events){
            if(!e||e.event!=='RUN_COMPLETED'||!/^[1-9]\d*$/.test(String(e.session_id)))continue;
            const id=String(e.session_id);
            if(!seen.has(id)){seen.add(id);sessions.push(id)}
        }
        pages++;
        if(pages>4096)throw new Error('winding_history_page_limit');
        if(j.has_more!==true)break;
        const next=Number(j.next_cursor);
        if(!Number.isSafeInteger(next)||next<=cursor)throw new Error('invalid_winding_history_cursor');
        cursor=next;
    }
    return sessions;
}

async function readSelection(sessionId){
    const r=await fetch('/api/jobs/spool-selection?session_id='+encodeURIComponent(sessionId),{cache:'no-store'});
    let j={};
    try{j=await r.json()}catch(_){throw new Error('invalid_spool_selection_response')}
    if(r.status===404)return{legacy:true,session_id:sessionId};
    if(!r.ok)throw new Error(j.error||'spool_selection_read_failed');
    if(String(j.session_id)!==sessionId||String(j.repair_id)!==String(repairId)||
       !/^[1-9]\d*$/.test(String(j.spool_id))||j.automatic_writeoff_allowed!==false){
        throw new Error('spool_selection_identity_mismatch');
    }
    return j;
}

async function run(){
    try{
        const sessions=await loadCompletedSessions();
        if(!sessions.length){box.className='note muted';box.textContent='Для завершённых сессий бухты ещё не зафиксированы: RUN_COMPLETED отсутствует.';return}
        const rows=[];
        for(const sessionId of sessions){
            const s=await readSelection(sessionId);
            if(s.legacy){rows.push('<div>Сессия №'+esc(sessionId)+' · <span class="muted">legacy: spool-selection отсутствует</span></div>');continue}
            rows.push('<div><b>Сессия №'+esc(sessionId)+' → бухта №'+esc(s.spool_id)+'</b> · '+esc(s.wire_type)+' · '+diameter(s.diameter_hundredths_mm)+' мм · вес при выборе '+kg(s.weight_at_selection_g)+' кг</div>');
        }
        box.className='note';
        box.innerHTML='<b>Бухты завершённых сессий</b><div class="muted">Read-only immutable metadata. RUN_COMPLETED не выполняет списание.</div>'+rows.join('');
    }catch(e){
        box.className='note warning';
        box.textContent='Не удалось полностью подтвердить spool-selection истории: '+(e.message||'unknown')+'. Данные о бухтах не считаются подтверждёнными.';
    }
}
run();
})();
