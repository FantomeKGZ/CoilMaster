(()=>{
'use strict';
const p=new URLSearchParams(location.search),repairId=p.get('repair_id')||localStorage.getItem('cm-active-repair')||'';
const spoolSelect=document.getElementById('spool');
if(!spoolSelect||!/^[1-9]\d*$/.test(repairId))return;

const note=document.createElement('div');
note.className='info muted';
note.textContent='Поиск завершённого прогона, который ещё не покрыт списанием…';
const spoolInfo=document.getElementById('spoolInfo');
if(spoolInfo&&spoolInfo.parentNode)spoolInfo.parentNode.insertBefore(note,spoolInfo);
else spoolSelect.insertAdjacentElement('afterend',note);

let suggestedSessionId='',suggestedRunId='',suggestedSpoolId='';

function clearProvenance(){
    suggestedSessionId='';
    suggestedRunId='';
    suggestedSpoolId='';
    delete document.body.dataset.writeoffSourceSessionId;
    delete document.body.dataset.writeoffSourceRunId;
    delete document.body.dataset.writeoffSourceSpoolId;
}
clearProvenance();

const protectedWriteoffMessage=code=>({
    source_session_and_run_required:'Для нового списания обязательны exact session-id и run-id завершённого прогона. Списание без provenance запрещено.',
    source_session_and_run_required_together:'Защищённая связь со списанием неполная: session-id и run-id должны передаваться вместе.',
    source_run_provenance_required:'Не выбран доказуемый завершённый прогон с immutable бухтой. Списание заблокировано.',
    invalid_source_session_or_run_id:'Некорректные идентификаторы сессии или прохода. Списание не выполнено; обновите страницу и проверьте историю намотки.',
    job_spool_selection_store_unavailable:'Хранилище immutable выбора бухты временно недоступно. Списание не выполнено; проверьте microSD.',
    job_spool_selection_read_failed:'Immutable запись выбора бухты не прошла проверку целостности. Списание заблокировано до устранения проблемы данных.',
    source_session_spool_selection_not_found:'Для выбранной сессии не найдена immutable запись бухты. Автоматическая подмена запрещена; требуется проверка данных.',
    source_session_spool_mismatch:'Выбранная бухта не совпадает с immutable бухтой этой сессии. Списание не выполнено; верните бухту, указанную в provenance.',
    winding_history_unavailable:'История намотки временно недоступна. Нельзя доказать завершение прохода, поэтому списание не выполнено.',
    winding_history_integrity_failed:'История намотки не прошла проверку целостности. Списание по provenance заблокировано.',
    source_run_not_completed:'Указанный проход не имеет подтверждённого RUN_COMPLETED. Списание для него не разрешено.',
    source_run_writeoff_lookup_failed:'Не удалось доказуемо проверить, не списан ли этот проход ранее. Повторное списание заблокировано.',
    source_run_already_written_off:'Для этой сессии и прохода уже существует подтверждённое ручное списание. Повторять операцию нельзя.',
    write_off_not_committed:'Складская транзакция не была подтверждена. Не считайте расход списанным; обновите остаток и историю перед следующей попыткой.'
})[code]||'';

function blockedResponse(code,message){
    note.className='warning';
    note.textContent=message;
    return new Response(JSON.stringify({error:message,error_code:code,write_performed:false}),{
        status:409,
        headers:{'Content-Type':'application/json; charset=utf-8'}
    });
}

function syncProvenance(){
    if(suggestedSessionId&&suggestedRunId&&suggestedSpoolId&&spoolSelect.value===suggestedSpoolId){
        document.body.dataset.writeoffSourceSessionId=suggestedSessionId;
        document.body.dataset.writeoffSourceRunId=suggestedRunId;
        document.body.dataset.writeoffSourceSpoolId=suggestedSpoolId;
        return true;
    }
    delete document.body.dataset.writeoffSourceSessionId;
    delete document.body.dataset.writeoffSourceRunId;
    delete document.body.dataset.writeoffSourceSpoolId;
    if(suggestedSessionId&&suggestedRunId&&spoolSelect.value&&spoolSelect.value!==suggestedSpoolId){
        note.className='warning';
        note.textContent='Для сессии №'+suggestedSessionId+', проход №'+suggestedRunId+' разрешена только immutable бухта №'+suggestedSpoolId+'. Выбранная вручную другая бухта не будет списана.';
    }
    return false;
}
spoolSelect.addEventListener('change',syncProvenance);

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
            if(!/^[1-9]\d*$/.test(sessionId)||!/^[1-9]\d*$/.test(runId)||sourceSpoolId!==postedSpoolId){
                return blockedResponse('source_run_provenance_required',protectedWriteoffMessage('source_run_provenance_required'));
            }
            init.body.set('source_session_id',sessionId);
            init.body.set('source_run_id',runId);
            attachedSessionId=sessionId;
            attachedRunId=runId;
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

        if(isWriteoffPost&&attachedSessionId&&attachedRunId&&response.ok){
            try{
                const payload=await response.clone().json();
                if(payload&&payload.confirmed===true&&String(payload.source_session_id)===attachedSessionId&&String(payload.source_run_id)===attachedRunId){
                    note.className='info';
                    note.textContent='Ручное списание подтверждено для сессии №'+attachedSessionId+', проход №'+attachedRunId+'. Ищу следующий незакрытый завершённый прогон…';
                    clearProvenance();
                    setTimeout(run,600);
                }
            }catch(_){}
        }
        return response;
    };
    window.cmWriteoffProvenanceFetchWrapped=true;
}

const escText=v=>String(v??'');
const sleep=ms=>new Promise(resolve=>setTimeout(resolve,ms));

async function loadCompletedRuns(){
    const completed=[];
    const seen=new Set();
    let cursor=0,pages=0;
    for(;;){
        const r=await fetch('/api/winding-history?repair_id='+encodeURIComponent(repairId)+'&cursor='+cursor+'&limit=100',{cache:'no-store'});
        const j=await r.json();
        if(!r.ok)throw new Error(j.error||'winding_history_read_failed');
        const page=Array.isArray(j.events)?j.events:[];
        for(const e of page){
            if(!e||e.event!=='RUN_COMPLETED')continue;
            const sessionId=String(e.session_id||''),runId=String(e.run_id||'');
            if(!/^[1-9]\d*$/.test(sessionId)||!/^[1-9]\d*$/.test(runId))throw new Error('invalid_completed_run_identity');
            const key=sessionId+':'+runId;
            if(seen.has(key))throw new Error('duplicate_completed_run_identity');
            seen.add(key);
            completed.push({sessionId,runId,key});
        }
        pages++;
        if(pages>4096)throw new Error('winding_history_page_limit');
        if(j.has_more!==true)break;
        const next=Number(j.next_cursor);
        if(!Number.isSafeInteger(next)||next<=cursor)throw new Error('invalid_winding_history_cursor');
        cursor=next;
    }
    return completed;
}

async function loadWriteoffCoverage(){
    const r=await fetch('/api/warehouse/write-offs?repair_id='+encodeURIComponent(repairId),{cache:'no-store'});
    let j={};
    try{j=await r.json()}catch(_){throw new Error('writeoff_history_invalid_response')}
    if(!r.ok)throw new Error(j.error||'writeoff_history_read_failed');
    if(!Array.isArray(j.items))throw new Error('writeoff_history_items_missing');

    const exact=new Set(),legacySessions=new Set();
    for(const item of j.items){
        const sourceSession=item&&item.source_session_id;
        if(sourceSession===null||sourceSession===undefined)continue;
        const sessionId=String(sourceSession);
        if(!/^[1-9]\d*$/.test(sessionId))throw new Error('invalid_writeoff_source_session_id');
        const sourceRun=item.source_run_id;
        if(sourceRun===null||sourceRun===undefined){
            if(legacySessions.has(sessionId))throw new Error('duplicate_legacy_writeoff_provenance');
            legacySessions.add(sessionId);
            continue;
        }
        const runId=String(sourceRun);
        if(!/^[1-9]\d*$/.test(runId))throw new Error('invalid_writeoff_source_run_id');
        const key=sessionId+':'+runId;
        if(exact.has(key))throw new Error('duplicate_writeoff_provenance');
        exact.add(key);
    }
    return{exact,legacySessions};
}

function nextUncoveredRun(completed,coverage){
    for(let i=completed.length-1;i>=0;--i){
        const candidate=completed[i];
        if(coverage.exact.has(candidate.key))continue;
        if(coverage.legacySessions.has(candidate.sessionId))throw new Error('legacy_session_writeoff_requires_review');
        return candidate;
    }
    return null;
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

function explainFailure(code){
    return({
        legacy_session_writeoff_requires_review:'Найдена старая session-only запись списания без run-id. Она оставлена для чтения, но новое exact-run списание в этой сессии автоматически не создаётся: требуется ручная проверка исторических данных.',
        spool_selection_identity_mismatch:'Immutable spool-selection не совпадает с ремонтом или содержит некорректную identity. Списание заблокировано.',
        spool_selection_read_failed:'Не удалось прочитать immutable spool-selection. Списание заблокировано.',
        writeoff_history_read_failed:'Не удалось прочитать историю списаний. Нельзя доказать отсутствие дубля, поэтому списание заблокировано.',
        winding_history_read_failed:'Не удалось прочитать историю намотки. Нельзя доказать RUN_COMPLETED, поэтому списание заблокировано.'
    })[code]||('Не удалось доказуемо подготовить provenance списания ('+escText(code||'unknown')+'). Списание заблокировано.');
}

async function run(){
    clearProvenance();
    note.className='info muted';
    note.textContent='Поиск завершённого прогона, который ещё не покрыт списанием…';
    try{
        const completed=await loadCompletedRuns();
        if(!completed.length){
            note.className='warning';
            note.textContent='В истории ремонта нет подтверждённого RUN_COMPLETED. Новое списание провода без exact run provenance запрещено.';
            return;
        }

        const coverage=await loadWriteoffCoverage();
        const pending=nextUncoveredRun(completed,coverage);
        if(!pending){
            note.className='info';
            note.textContent='Все подтверждённые RUN_COMPLETED этого ремонта уже покрыты exact ручными списаниями. Новое списание не требуется.';
            return;
        }

        const {sessionId,runId}=pending;
        const selection=await loadSelection(sessionId);
        if(!selection){
            note.className='warning';
            note.textContent='Для сессии №'+sessionId+', проход №'+runId+' нет immutable spool-selection. Это legacy/неполные данные; новое списание заблокировано до ручной проверки.';
            return;
        }
        if(!await waitForSpools())throw new Error('spool_catalog_not_loaded');

        const spoolId=String(selection.spool_id);
        const option=[...spoolSelect.options].find(o=>o.value===spoolId);
        if(!option){
            note.className='warning';
            note.textContent='Сессия №'+sessionId+', проход №'+runId+' выполнялись с бухтой №'+spoolId+', но эта бухта сейчас не доступна среди активных. Автоматической замены нет; списание заблокировано до проверки склада.';
            return;
        }

        suggestedSessionId=sessionId;
        suggestedRunId=runId;
        suggestedSpoolId=spoolId;
        spoolSelect.value=spoolId;
        spoolSelect.dispatchEvent(new Event('change'));
        syncProvenance();
        note.className='info';
        note.textContent='Готово ручное списание для сессии №'+sessionId+', проход №'+runId+', exact бухта №'+spoolId+'. Введите фактический вес после работы и нажмите подтверждение; автоматического списания нет.';
    }catch(e){
        clearProvenance();
        note.className='warning';
        note.textContent=explainFailure(e&&e.message);
    }
}

run();
})();