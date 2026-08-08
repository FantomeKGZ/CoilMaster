(()=>{
'use strict';
const p=new URLSearchParams(location.search),repairId=p.get('repair_id')||localStorage.getItem('cm-active-repair')||'';
const spoolSelect=document.getElementById('spool');
if(!spoolSelect||!/^[1-9]\d*$/.test(repairId))return;

delete document.body.dataset.writeoffSourceSessionId;
delete document.body.dataset.writeoffSourceRunId;
delete document.body.dataset.writeoffSourceSpoolId;

const note=document.createElement('div');
note.className='info muted';
note.textContent='Проверка бухты из последнего завершённого прогона…';
const spoolInfo=document.getElementById('spoolInfo');
if(spoolInfo&&spoolInfo.parentNode)spoolInfo.parentNode.insertBefore(note,spoolInfo);
else spoolSelect.insertAdjacentElement('afterend',note);

const protectedWriteoffMessage=code=>({
    source_session_and_run_required_together:'Защищённая связь со списанием неполная: session-id и run-id должны передаваться вместе. Обновите страницу и повторите выбор завершённого прогона.',
    invalid_source_session_or_run_id:'Некорректные идентификаторы сессии или прохода. Списание не выполнено; обновите страницу и проверьте историю намотки.',
    job_spool_selection_store_unavailable:'Хранилище immutable выбора бухты временно недоступно. Списание не выполнено; проверьте microSD.',
    job_spool_selection_read_failed:'Immutable запись выбора бухты не прошла проверку целостности. Списание заблокировано до устранения проблемы данных.',
    source_session_spool_selection_not_found:'Для выбранной сессии не найдена immutable запись бухты. Автоматическая подмена запрещена; проверьте историю задания.',
    source_session_spool_mismatch:'Выбранная бухта не совпадает с immutable бухтой этой сессии. Списание не выполнено; не заменяйте бухту автоматически.',
    winding_history_unavailable:'История намотки временно недоступна. Нельзя доказать завершение прохода, поэтому списание не выполнено.',
    winding_history_integrity_failed:'История намотки не прошла проверку целостности. Списание по provenance заблокировано.',
    source_run_not_completed:'Указанный проход не имеет подтверждённого RUN_COMPLETED. Списание для него не разрешено.',
    source_run_writeoff_lookup_failed:'Не удалось доказуемо проверить, не списан ли этот проход ранее. Повторное списание заблокировано.',
    source_run_already_written_off:'Для этой сессии и прохода уже существует подтверждённое ручное списание. Повторять операцию нельзя; обновите историю.',
    write_off_not_committed:'Складская транзакция не была подтверждена. Не считайте расход списанным; обновите остаток и историю перед следующей попыткой.'
})[code]||'';

if(!window.cmWriteoffProvenanceFetchWrapped){
    const originalFetch=window.fetch.bind(window);
    window.fetch=async(input,init)=>{
        const url=typeof input==='string'?input:(input&&input.url)||'';
        const isWriteoffPost=url==='/api/warehouse/write-offs'&&init&&String(init.method||'GET').toUpperCase()==='POST'&&init.body instanceof URLSearchParams;
        let attachedSessionId='',attachedRunId='';
        if(isWriteoffPost){
            const sessionId=document.body.dataset.writeoffSourceSessionId||'';
            const runId=document.body.dataset.writeoffSourceRunId||'';
            const sourceSpoolId=document.body.dataset.writeoffSourceSpoolId||'';
            const postedSpoolId=String(init.body.get('spool_id')||'');
            if(/^[1-9]\d*$/.test(sessionId)&&/^[1-9]\d*$/.test(runId)&&sourceSpoolId===postedSpoolId){
                init.body.set('source_session_id',sessionId);
                init.body.set('source_run_id',runId);
                attachedSessionId=sessionId;
                attachedRunId=runId;
            }else{
                init.body.delete('source_session_id');
                init.body.delete('source_run_id');
            }
        }
        const response=await originalFetch(input,init);
        if(isWriteoffPost&&!response.ok){
            try{
                const payload=await response.clone().json();
                const code=String(payload&&payload.error||'');
                const message=protectedWriteoffMessage(code);
                if(message){
                    note.className='warning';
                    note.textContent=message;
                    const translated=Object.assign({},payload,{error:message,error_code:code});
                    return new Response(JSON.stringify(translated),{
                        status:response.status,
                        statusText:response.statusText,
                        headers:new Headers(response.headers)
                    });
                }
            }catch(_){}
        }
        if(attachedSessionId&&attachedRunId&&response.ok){
            try{
                const payload=await response.clone().json();
                if(payload&&payload.confirmed===true&&String(payload.source_session_id)===attachedSessionId&&String(payload.source_run_id)===attachedRunId){
                    note.className='info';
                    note.textContent='Ручное списание подтверждено для сессии №'+attachedSessionId+', проход №'+attachedRunId+'. Этот прогон теперь покрыт складским provenance.';
                    delete document.body.dataset.writeoffSourceSessionId;
                    delete document.body.dataset.writeoffSourceRunId;
                    delete document.body.dataset.writeoffSourceSpoolId;
                }
            }catch(_){}
        }
        return response;
    };
    window.cmWriteoffProvenanceFetchWrapped=true;
}

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
    let exactMatches=0,legacyMatches=0;
    for(const item of j.items){
        const sourceSession=item&&item.source_session_id;
        if(sourceSession===null||sourceSession===undefined)continue;
        if(!/^[1-9]\d*$/.test(String(sourceSession)))throw new Error('invalid_writeoff_source_session_id');
        if(String(sourceSession)!==sessionId)continue;
        const sourceRun=item.source_run_id;
        if(sourceRun===null||sourceRun===undefined){legacyMatches++;continue;}
        if(!/^[1-9]\d*$/.test(String(sourceRun)))throw new Error('invalid_writeoff_source_run_id');
        if(String(sourceRun)===runId)exactMatches++;
    }
    if(legacyMatches>1||exactMatches>1||(legacyMatches&&exactMatches))throw new Error('duplicate_writeoff_provenance');
    return legacyMatches===1||exactMatches===1;
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
        note.textContent='Предложена бухта №'+spoolId+' из immutable записи: сессия №'+sessionId+', проход №'+runId+'. Пока выбрана эта бухта, оба идентификатора будут сохранены как provenance ручного списания. Расход и подтверждение остаются ручными.';
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
