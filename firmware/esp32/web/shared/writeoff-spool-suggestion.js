(()=>{
'use strict';
const p=new URLSearchParams(location.search),repairId=p.get('repair_id')||localStorage.getItem('cm-active-repair')||'';
const spoolSelect=document.getElementById('spool');
if(!spoolSelect||!/^[1-9]\d*$/.test(repairId))return;

const note=document.createElement('div');
note.className='info muted';
note.textContent='Проверка бухты из последней завершённой намотки…';
const spoolInfo=document.getElementById('spoolInfo');
if(spoolInfo&&spoolInfo.parentNode)spoolInfo.parentNode.insertBefore(note,spoolInfo);
else spoolSelect.insertAdjacentElement('afterend',note);

const escText=v=>String(v??'');
const sleep=ms=>new Promise(resolve=>setTimeout(resolve,ms));

async function latestCompletedSession(){
    const events=[];
    let cursor=0,pages=0;
    for(;;){
        const r=await fetch('/api/winding-history?repair_id='+encodeURIComponent(repairId)+'&cursor='+cursor+'&limit=100',{cache:'no-store'});
        const j=await r.json();
        if(!r.ok)throw new Error(j.error||'winding_history_read_failed');
        const page=Array.isArray(j.events)?j.events:[];
        events.push(...page);
        pages++;
        if(pages>4096)throw new Error('winding_history_page_limit');
        if(j.has_more!==true)break;
        const next=Number(j.next_cursor);
        if(!Number.isSafeInteger(next)||next<=cursor)throw new Error('invalid_winding_history_cursor');
        cursor=next;
    }
    for(let i=events.length-1;i>=0;--i){
        const e=events[i];
        if(e&&e.event==='RUN_COMPLETED'&&/^[1-9]\d*$/.test(String(e.session_id)))return String(e.session_id);
    }
    return '';
}

async function loadSelection(sessionId){
    const r=await fetch('/api/jobs/spool-selection?session_id='+encodeURIComponent(sessionId),{cache:'no-store'});
    let j={};
    try{j=await r.json()}catch(_){throw new Error('spool_selection_invalid_response')}
    if(r.status===404)return null;
    if(!r.ok)throw new Error(j.error||'spool_selection_read_failed');
    if(String(j.session_id)!==sessionId||String(j.repair_id)!==String(repairId)||
       !/^[1-9]\d*$/.test(String(j.spool_id))||j.automatic_writeoff_allowed!==false){
        throw new Error('spool_selection_identity_mismatch');
    }
    return j;
}

async function waitForSpools(){
    for(let i=0;i<50;i++){
        if(spoolSelect.options.length>0&&spoolSelect.options[0].textContent!=='Загрузка…')return true;
        await sleep(200);
    }
    return false;
}

async function run(){
    try{
        const sessionId=await latestCompletedSession();
        if(!sessionId){
            note.className='info muted';
            note.textContent='В истории ремонта нет подтверждённого RUN_COMPLETED. Бухта выбирается вручную.';
            return;
        }
        const selection=await loadSelection(sessionId);
        if(!selection){
            note.className='info muted';
            note.textContent='Последняя завершённая сессия №'+sessionId+' создана без immutable spool-selection (legacy). Бухта выбирается вручную.';
            return;
        }
        if(!await waitForSpools())throw new Error('spool_catalog_not_loaded');
        const spoolId=String(selection.spool_id);
        const option=[...spoolSelect.options].find(o=>o.value===spoolId);
        if(!option){
            note.className='warning';
            note.textContent='Сессия №'+sessionId+' была выполнена с бухтой №'+spoolId+', но эта бухта сейчас не доступна среди активных. Автоматической замены нет — проверьте склад и выберите бухту вручную.';
            return;
        }
        spoolSelect.value=spoolId;
        spoolSelect.dispatchEvent(new Event('change'));
        note.className='info';
        note.textContent='Предложена бухта №'+spoolId+' из immutable записи завершённой сессии №'+sessionId+'. Это только подсказка: фактический вес после работы и подтверждение списания остаются ручными.';
    }catch(e){
        note.className='warning';
        note.textContent='Не удалось доказуемо получить бухту из завершённой намотки ('+escText(e.message||'unknown')+'). Автоматический выбор не выполнен; ручное списание остаётся доступным.';
    }
}

run();
})();
