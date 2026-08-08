(()=>{
'use strict';
const p=new URLSearchParams(location.search),repairId=p.get('repair_id')||localStorage.getItem('cm-active-repair')||'';
const spoolSelect=document.getElementById('spool');
if(!spoolSelect||!/^[1-9]\d*$/.test(repairId))return;

delete document.body.dataset.writeoffSourceSessionId;
delete document.body.dataset.writeoffSourceRunId;
delete document.body.dataset.writeoffSourceSpoolId;

if(!window.cmWriteoffProvenanceFetchWrapped){
    const originalFetch=window.fetch.bind(window);
    window.fetch=(input,init)=>{
        const url=typeof input==='string'?input:(input&&input.url)||'';
        if(url==='/api/warehouse/write-offs'&&init&&String(init.method||'GET').toUpperCase()==='POST'&&init.body instanceof URLSearchParams){
            const sessionId=document.body.dataset.writeoffSourceSessionId||'';
            const runId=document.body.dataset.writeoffSourceRunId||'';
            const sourceSpoolId=document.body.dataset.writeoffSourceSpoolId||'';
            const postedSpoolId=String(init.body.get('spool_id')||'');
            if(/^[1-9]\d*$/.test(sessionId)&&/^[1-9]\d*$/.test(runId)&&sourceSpoolId===postedSpoolId){
                init.body.set('source_session_id',sessionId);
                init.body.set('source_run_id',runId);
            }else{
                init.body.delete('source_session_id');
                init.body.delete('source_run_id');
            }
        }
        return originalFetch(input,init);
    };
    window.cmWriteoffProvenanceFetchWrapped=true;
}

const note=document.createElement('div');
note.className='info muted';
note.textContent='Проверка бухты из последнего завершённого прогона…';
const spoolInfo=document.getElementById('spoolInfo');
if(spoolInfo&&spoolInfo.parentNode)spoolInfo.parentNode.insertBefore(note,spoolInfo);
else spoolSelect.insertAdjacentElement('afterend',note);

const escText=v=>String(v??'');
const sleep=ms=>new Promise(resolve=>setTimeout(resolve,ms));
let suggestedSessionId='',suggestedRunId='',suggestedSpoolId='';

function syncProvenance(){
    if(suggestedSessionId&&suggestedRunId&&suggestedSpoolId&&spoolSelect.value===suggestedSpoolId){
        document.body.dataset.writeoffSourceSessionId=suggestedSessionId;
        document.body.dataset.writeoffSourceRunId=suggestedRunId;
        document.body.dataset.writeoffSourceSpoolId=suggestedSpoolId;
        return;
    }
    delete document.body.dataset.writeoffSourceSessionId;
    delete document.body.dataset.writeoffSourceRunId;
    delete document.body.dataset.writeoffSourceSpoolId;
}
spoolSelect.addEventListener('change',syncProvenance);

async function latestCompletedRun(){
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
        const sessionId=String(e&&e.session_id||''),runId=String(e&&e.run_id||'');
        if(e&&e.event==='RUN_COMPLETED'&&/^[1-9]\d*$/.test(sessionId)&&/^[1-9]\d*$/.test(runId))return{sessionId,runId};
    }
    return null;
}

async function runAlreadyWrittenOff(sessionId,runId){
    const r=await fetch('/api/warehouse/write-offs?repair_id='+encodeURIComponent(repairId),{cache:'no-store'});
    let j={};
    try{j=await r.json()}catch(_){throw new Error('writeoff_history_invalid_response')}
    if(!r.ok)throw new Error(j.error||'writeoff_history_read_failed');
    if(!Array.isArray(j.items))throw new Error('writeoff_history_items_missing');
    for(const item of j.items){
        const sourceSession=item&&item.source_session_id;
        if(sourceSession===null||sourceSession===undefined)continue;
        if(!/^[1-9]\d*$/.test(String(sourceSession)))throw new Error('invalid_writeoff_source_session_id');
        if(String(sourceSession)!==sessionId)continue;
        const sourceRun=item.source_run_id;
        if(sourceRun===null||sourceRun===undefined)return true;
        if(!/^[1-9]\d*$/.test(String(sourceRun)))throw new Error('invalid_writeoff_source_run_id');
        if(String(sourceRun)===runId)return true;
    }
    return false;
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
        const completed=await latestCompletedRun();
        if(!completed){
            note.className='info muted';
            note.textContent='В истории ремонта нет подтверждённого RUN_COMPLETED. Бухта выбирается вручную.';
            return;
        }
        const {sessionId,runId}=completed;
        if(await runAlreadyWrittenOff(sessionId,runId)){
            note.className='info muted';
            note.textContent='Для последнего завершённого прогона: сессия №'+sessionId+', проход №'+runId+' — уже есть подтверждённое ручное списание. Повторная подсказка отключена.';
            return;
        }
        const selection=await loadSelection(sessionId);
        if(!selection){
            note.className='info muted';
            note.textContent='Сессия №'+sessionId+' создана без immutable spool-selection (legacy). Бухта выбирается вручную.';
            return;
        }
        if(!await waitForSpools())throw new Error('spool_catalog_not_loaded');
        const spoolId=String(selection.spool_id);
        const option=[...spoolSelect.options].find(o=>o.value===spoolId);
        if(!option){
            note.className='warning';
            note.textContent='Сессия №'+sessionId+', проход №'+runId+' выполнялись с бухтой №'+spoolId+', но эта бухта сейчас не доступна среди активных. Автоматической замены нет — проверьте склад и выберите бухту вручную.';
            return;
        }
        suggestedSessionId=sessionId;
        suggestedRunId=runId;
        suggestedSpoolId=spoolId;
        spoolSelect.value=spoolId;
        spoolSelect.dispatchEvent(new Event('change'));
        syncProvenance();
        note.className='info';
        note.textContent='Предложена бухта №'+spoolId+' из immutable записи сессии №'+sessionId+', проход №'+runId+'. Пока выбрана эта бухта, session/run будут сохранены как provenance ручного списания. Расход и подтверждение остаются ручными.';
    }catch(e){
        suggestedSessionId='';
        suggestedRunId='';
        suggestedSpoolId='';
        syncProvenance();
        note.className='warning';
        note.textContent='Не удалось доказуемо получить безопасную подсказку бухты ('+escText(e.message||'unknown')+'). Автоматический выбор не выполнен; ручное списание остаётся доступным.';
    }
}

run();
})();
