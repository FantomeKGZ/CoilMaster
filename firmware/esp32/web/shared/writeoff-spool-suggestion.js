(()=>{
'use strict';
const $=id=>document.getElementById(id);
const params=new URLSearchParams(location.search);
const repairId=params.get('repair_id')||localStorage.getItem('cm-active-repair')||'';
const validId=value=>/^[1-9]\d*$/.test(String(value||''));
const esc=value=>String(value??'').replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
const kgFromGrams=value=>(Number(value||0)/1000).toLocaleString('ru-RU',{minimumFractionDigits:3,maximumFractionDigits:3})+' кг';
const diameterLabel=value=>(Number(value||0)/100).toLocaleString('ru-RU',{minimumFractionDigits:2,maximumFractionDigits:2})+' мм';
const money=(value,currency='KGS')=>(Number(value||0)/100).toLocaleString('ru-RU',{minimumFractionDigits:2,maximumFractionDigits:2})+' '+(currency==='KGS'?'сом':currency);
const materialLabel=value=>value==='CU'?'Медь':value==='AL'?'Алюминий':'Материал не указан';
const dateLabel=value=>{const d=new Date(value);return Number.isNaN(d.getTime())?String(value||''):d.toLocaleString('ru-RU')};

if(!validId(repairId)){
    if($('result')){$('result').className='bad';$('result').textContent='Откройте списание из карточки ремонта.'}
    return;
}
localStorage.setItem('cm-active-repair',repairId);
if($('repairLabel'))$('repairLabel').textContent='№'+repairId;

let writable=false;
let sourceSessionId='';
let sourceRunId='';
let selection=null;
let activeSpool=null;
let spoolBridge=null;
let materialRequests=[];
let selectedMaterialRequestId='';
let historyCursor=0;
let historyNextCursor=0;
let historyHasMore=false;
let historyStack=[];
let historyPage=1;

function setResult(text,kind='muted'){
    if(!$('result'))return;
    $('result').className=kind;
    $('result').textContent=text;
}
function setLifecycle(text,ok){
    if(!$('lifecycle'))return;
    $('lifecycle').className=ok?'info':'warning';
    $('lifecycle').textContent=text;
}
function canSubmit(){
    return writable&&validId(sourceSessionId)&&validId(sourceRunId)&&selection&&activeSpool&&spoolBridge&&validId(selectedMaterialRequestId);
}
function refreshSubmitState(){
    if($('submitButton'))$('submitButton').disabled=!canSubmit();
    if($('materialRequestId'))$('materialRequestId').disabled=!writable||!activeSpool||!spoolBridge||materialRequests.length===0;
}
function setFormEnabled(enabled){
    writable=enabled;
    ['quantityKg','comment'].forEach(id=>{const el=$(id);if(el)el.disabled=!enabled});
    refreshAllocationControls();
    refreshSubmitState();
}
function canonicalKg(input){
    const raw=String(input??'').trim().replace(',','.');
    if(!/^(?:0|[1-9]\d*)(?:\.\d{1,3})?$/.test(raw))return null;
    const parts=raw.split('.');
    const whole=parts[0];
    let fraction=parts[1]||'';
    const grams=Number(whole)*1000+Number((fraction+'000').slice(0,3));
    if(!Number.isSafeInteger(grams)||grams<=0||grams>0xFFFFFFFF)return null;
    fraction=fraction.replace(/0+$/,'');
    return{kg:fraction?whole+'.'+fraction:whole,grams};
}

async function jsonFetch(url,options){
    const response=await fetch(url,options);
    let payload={};
    try{payload=await response.json()}catch(_){throw new Error('invalid_json_response')}
    if(!response.ok){const error=new Error(payload.error||('http_'+response.status));error.status=response.status;error.payload=payload;throw error}
    return payload;
}

async function loadLifecycle(){
    try{
        const data=await jsonFetch('/api/repairs/by-id?repair_id='+encodeURIComponent(repairId),{cache:'no-store'});
        const repair=data.item;
        if(!repair||String(repair.repair_id)!==String(repairId))throw new Error('repair_not_found');
        const closed=(repair.current_status||repair.status)==='CLOSED';
        setLifecycle(closed?'Ремонт закрыт. История доступна, новые списания запрещены.':'Ремонт открыт. Ручное списание разрешено.',!closed);
        setFormEnabled(!closed);
    }catch(_){
        setLifecycle('Не удалось подтвердить состояние ремонта. Списание заблокировано.',false);
        setFormEnabled(false);
    }
}

async function loadCompletedRuns(){
    const completed=[];
    const seen=new Set();
    let cursor=0;
    for(let page=0;page<4096;page++){
        const data=await jsonFetch('/api/winding-history?repair_id='+encodeURIComponent(repairId)+'&cursor='+cursor+'&limit=100',{cache:'no-store'});
        for(const event of Array.isArray(data.events)?data.events:[]){
            if(!event||event.event!=='RUN_COMPLETED')continue;
            const sessionId=String(event.session_id||''),runId=String(event.run_id||'');
            if(!validId(sessionId)||!validId(runId))throw new Error('invalid_completed_run_identity');
            const key=sessionId+':'+runId;
            if(seen.has(key))throw new Error('duplicate_completed_run_identity');
            seen.add(key);
            completed.push({sessionId,runId,key});
        }
        if(data.has_more!==true)return completed;
        const next=Number(data.next_cursor);
        if(!Number.isSafeInteger(next)||next<=cursor)throw new Error('invalid_winding_history_cursor');
        cursor=next;
    }
    throw new Error('winding_history_page_limit');
}

async function loadWriteoffCoverage(){
    const exact=new Set(),legacySessions=new Set();
    let cursor=0;
    for(let page=0;page<4096;page++){
        const q=new URLSearchParams({repair_id:repairId,limit:'32'});
        if(cursor)q.set('cursor',String(cursor));
        const data=await jsonFetch('/api/warehouse/write-offs?'+q,{cache:'no-store'});
        for(const item of Array.isArray(data.items)?data.items:[]){
            const session=item&&item.source_session_id;
            if(session===null||session===undefined)continue;
            const sessionId=String(session);
            if(!validId(sessionId))throw new Error('invalid_writeoff_source_session_id');
            const run=item.source_run_id;
            if(run===null||run===undefined){
                if(legacySessions.has(sessionId))throw new Error('duplicate_legacy_writeoff_provenance');
                legacySessions.add(sessionId);
            }else{
                const runId=String(run);
                if(!validId(runId))throw new Error('invalid_writeoff_source_run_id');
                const key=sessionId+':'+runId;
                if(exact.has(key))throw new Error('duplicate_writeoff_provenance');
                exact.add(key);
            }
        }
        if(data.has_more!==true)return{exact,legacySessions};
        const next=Number(data.next_cursor);
        if(!Number.isSafeInteger(next)||next<=cursor)throw new Error('invalid_writeoff_cursor');
        cursor=next;
    }
    throw new Error('writeoff_page_limit');
}

function nextUncoveredRun(completed,coverage){
    for(let index=completed.length-1;index>=0;--index){
        const candidate=completed[index];
        if(coverage.exact.has(candidate.key))continue;
        if(coverage.legacySessions.has(candidate.sessionId))throw new Error('legacy_session_writeoff_requires_review');
        return candidate;
    }
    return null;
}

async function loadSelection(sessionId){
    const response=await fetch('/api/jobs/spool-selection?session_id='+encodeURIComponent(sessionId),{cache:'no-store'});
    let payload={};
    try{payload=await response.json()}catch(_){throw new Error('spool_selection_invalid_response')}
    if(response.status===404)return null;
    if(!response.ok)throw new Error(payload.error||'spool_selection_read_failed');
    if(String(payload.session_id)!==sessionId||String(payload.repair_id)!==String(repairId)||!validId(payload.spool_id)||payload.automatic_writeoff_allowed!==false)
        throw new Error('spool_selection_identity_mismatch');
    return payload;
}

async function findActiveSpool(spoolId){
    let cursor=0;
    for(let page=0;page<4096;page++){
        const q=new URLSearchParams({material:'ALL',limit:'32'});
        if(cursor)q.set('cursor',String(cursor));
        const data=await jsonFetch('/api/warehouse/spools?'+q,{cache:'no-store'});
        const found=(Array.isArray(data.items)?data.items:[]).find(item=>String(item.spool_id)===String(spoolId));
        if(found)return found.material_class==='CU'||found.material_class==='AL'?found:null;
        if(data.has_more!==true)return null;
        const next=Number(data.next_cursor);
        if(!Number.isSafeInteger(next)||next<=cursor)throw new Error('invalid_spool_cursor');
        cursor=next;
    }
    throw new Error('spool_page_limit');
}

function resetMaterialRequestContext(message='Сначала требуется exact RUN_COMPLETED и immutable бухта.'){
    spoolBridge=null;
    materialRequests=[];
    selectedMaterialRequestId='';
    if($('materialRequestId')){
        $('materialRequestId').innerHTML='<option value="">Заявка недоступна</option>';
        $('materialRequestId').value='';
        $('materialRequestId').disabled=true;
    }
    if($('materialRequestInfo')){
        $('materialRequestInfo').className='info muted';
        $('materialRequestInfo').textContent=message;
    }
    refreshSubmitState();
}

async function loadEligibleMaterialRequests(){
    const requests=[];
    let cursor=0;
    for(let page=0;page<4096;page++){
        const q=new URLSearchParams({repair_id:repairId,limit:'24'});
        if(cursor)q.set('cursor',String(cursor));
        const data=await jsonFetch('/api/material-requests?'+q,{cache:'no-store'});
        for(const item of Array.isArray(data.items)?data.items:[]){
            if(!item||String(item.repair_id)!==String(repairId)||!validId(item.material_request_id))throw new Error('material_request_identity_mismatch');
            requests.push(item);
        }
        if(data.has_more!==true)break;
        const next=Number(data.next_cursor);
        if(!Number.isSafeInteger(next)||next<=cursor)throw new Error('invalid_material_request_cursor');
        cursor=next;
        if(page===4095)throw new Error('material_request_page_limit');
    }

    const eligible=[];
    for(const request of requests){
        const requestId=String(request.material_request_id);
        const status=await jsonFetch('/api/material-requests/status?material_request_id='+encodeURIComponent(requestId),{cache:'no-store'});
        if(String(status.material_request_id)!==requestId)throw new Error('material_request_status_identity_mismatch');
        if(status.status==='DRAFT'||status.status==='ISSUED')eligible.push({...request,current_status:status.status});
    }
    return eligible.reverse();
}

async function prepareMaterialRequestContext(){
    resetMaterialRequestContext('Проверка связи immutable бухты с MaterialLedger…');
    if(!activeSpool||!selection)throw new Error('run_wire_spool_context_missing');

    const bridge=await jsonFetch('/api/warehouse/spool-material-bridges?spool_id='+encodeURIComponent(activeSpool.spool_id),{cache:'no-store'});
    if(String(bridge.spool_id)!==String(activeSpool.spool_id)||!validId(bridge.warehouse_item_id)||
       bridge.wire_type!==activeSpool.material_class||Number(bridge.diameter_hundredths_mm)!==Number(activeSpool.diameter_hundredths_mm))
        throw new Error('spool_material_bridge_identity_mismatch');
    spoolBridge=bridge;
    materialRequests=await loadEligibleMaterialRequests();

    if(!$('materialRequestId'))throw new Error('material_request_control_missing');
    $('materialRequestId').innerHTML='<option value="">Выберите заявку материалов</option>'+materialRequests.map(item=>'<option value="'+esc(item.material_request_id)+'">Заявка №'+esc(item.material_request_id)+' · '+esc(item.current_status)+' · '+esc(dateLabel(item.created_at))+'</option>').join('');
    $('materialRequestId').value='';
    $('materialRequestId').disabled=!writable||materialRequests.length===0;

    if($('materialRequestInfo')){
        if(materialRequests.length){
            $('materialRequestInfo').className='info';
            $('materialRequestInfo').textContent='Бухта №'+activeSpool.spool_id+' связана с MaterialLedger item №'+spoolBridge.warehouse_item_id+'. Выберите DRAFT/ISSUED заявку этого ремонта.';
        }else{
            $('materialRequestInfo').className='warning';
            $('materialRequestInfo').textContent='Для ремонта нет заявки материалов в статусе DRAFT или ISSUED. RUN_WIRE списание заблокировано.';
        }
    }
    refreshSubmitState();
}

if($('materialRequestId'))$('materialRequestId').onchange=()=>{
    const value=$('materialRequestId').value;
    selectedMaterialRequestId=materialRequests.some(item=>String(item.material_request_id)===String(value))?String(value):'';
    if($('materialRequestInfo')&&selectedMaterialRequestId){
        const selected=materialRequests.find(item=>String(item.material_request_id)===selectedMaterialRequestId);
        $('materialRequestInfo').className='info';
        $('materialRequestInfo').textContent='Заявка №'+selectedMaterialRequestId+' · '+selected.current_status+'; MaterialLedger item №'+spoolBridge.warehouse_item_id+'; бухта №'+activeSpool.spool_id+'.';
    }
    refreshSubmitState();
};

function refreshAllocationControls(){
    const unallocated=$('unallocatedFields');
    if(unallocated)unallocated.hidden=true;
    if($('wireType'))$('wireType').disabled=true;
    if($('diameterMm'))$('diameterMm').disabled=true;
    if($('allocationMode')){
        $('allocationMode').innerHTML=activeSpool?'<option value="SPOOL">Списать с immutable бухты №'+esc(activeSpool.spool_id)+'</option>':'<option value="SPOOL">Immutable бухта недоступна</option>';
        $('allocationMode').value='SPOOL';
        $('allocationMode').disabled=true;
    }
    if($('allocationInfo')){
        if(activeSpool){
            $('allocationInfo').className='info';
            $('allocationInfo').textContent='Будет уменьшен остаток immutable бухты №'+activeSpool.spool_id+': '+materialLabel(activeSpool.material_class)+' · '+diameterLabel(activeSpool.diameter_hundredths_mm)+' · '+kgFromGrams(activeSpool.current_weight_g)+'.';
        }else{
            $('allocationInfo').className='warning';
            $('allocationInfo').textContent='Immutable бухта source session недоступна среди активных. Списание заблокировано: менять provenance после RUN_COMPLETED нельзя.';
        }
    }
}

async function prepareNextRun(){
    sourceSessionId='';sourceRunId='';selection=null;activeSpool=null;
    resetMaterialRequestContext();
    if($('provenance')){$('provenance').className='info muted';$('provenance').textContent='Поиск завершённого прохода, который ещё не покрыт списанием…'}
    try{
        const completed=await loadCompletedRuns();
        const coverage=await loadWriteoffCoverage();
        const pending=nextUncoveredRun(completed,coverage);
        if(!pending){
            if($('provenance')){$('provenance').className='info';$('provenance').textContent='Все RUN_COMPLETED этого ремонта уже покрыты ручными списаниями.'}
            refreshAllocationControls();
            refreshSubmitState();
            return;
        }
        sourceSessionId=pending.sessionId;
        sourceRunId=pending.runId;
        selection=await loadSelection(sourceSessionId);
        if(!selection)throw new Error('spool_selection_missing');
        activeSpool=await findActiveSpool(selection.spool_id);
        if(!activeSpool)throw new Error('immutable_spool_not_active');
        if(String(activeSpool.spool_id)!==String(selection.spool_id)||
           (selection.wire_type&&selection.wire_type!==activeSpool.material_class)||
           (selection.diameter_hundredths_mm&&Number(selection.diameter_hundredths_mm)!==Number(activeSpool.diameter_hundredths_mm)))
            throw new Error('immutable_spool_identity_mismatch');
        if($('provenance')){
            $('provenance').className='info';
            $('provenance').textContent='Сессия №'+sourceSessionId+' · проход №'+sourceRunId+' · immutable бухта №'+selection.spool_id+'. Списание выполняется только вручную.';
        }
        if(selection.wire_type&&$('wireType'))$('wireType').value=selection.wire_type;
        if(selection.diameter_hundredths_mm&&$('diameterMm'))$('diameterMm').value=(Number(selection.diameter_hundredths_mm)/100).toFixed(2).replace('.',',');
        refreshAllocationControls();
        await prepareMaterialRequestContext();
        refreshSubmitState();
    }catch(error){
        refreshAllocationControls();
        resetMaterialRequestContext('RUN_WIRE заблокирован: '+error.message+'.');
        if($('provenance')){$('provenance').className='warning';$('provenance').textContent='Списание заблокировано: '+error.message+'. Проверьте историю намотки, immutable бухту, bridge и Material Request.'}
        refreshSubmitState();
    }
}

function historyModeLabel(item){
    if(item.writeoff_mode==='KG_FIRST')return item.stock_mode==='SPOOL'?'kg-first · бухта':'kg-first · без привязки';
    return 'legacy · бухта';
}
function renderBreakdown(totals,currency){
    if(!$('historyBreakdown'))return;
    const t=totals||{};
    $('historyBreakdown').innerHTML=['CU','AL','UNKNOWN'].map(key=>{const x=t[key]||{consumed_g:0,count:0,consumed_value_minor:0};return '<div><span class="muted">'+(key==='CU'?'Медь':key==='AL'?'Алюминий':'Без материала')+' · '+Number(x.count||0)+'</span><b>'+kgFromGrams(x.consumed_g)+'</b><span>'+money(x.consumed_value_minor||0,currency)+'</span></div>'}).join('');
}
function renderHistoryItem(item,currency){
    const spool=item.spool_id===null||item.spool_id===undefined?'без бухты':'бухта №'+esc(item.spool_id);
    const quantity=item.quantity_kg?String(item.quantity_kg).replace('.',',')+' кг':kgFromGrams(item.consumed_g);
    const weights=item.weight_before_g===null||item.weight_before_g===undefined?'':('<div class="muted">Остаток бухты: '+kgFromGrams(item.weight_before_g)+' → '+kgFromGrams(item.weight_after_g)+'</div>');
    const provenance=item.source_session_id===null||item.source_session_id===undefined?'':('<div class="source">Сессия №'+esc(item.source_session_id)+(item.source_run_id?(' · проход №'+esc(item.source_run_id)):' · legacy без run-id')+'</div>');
    return '<div class="history-item"><div class="history-head"><b>Движение №'+esc(item.movement_id)+' · '+spool+'</b><strong>'+quantity+' · '+money(item.consumed_value_minor||0,item.currency||currency)+'</strong></div><div><span class="badge">'+historyModeLabel(item)+'</span> · '+diameterLabel(item.diameter_hundredths_mm)+' · <span class="badge '+(item.legacy_unknown_material?'unknown':'')+'">'+materialLabel(item.wire_type)+'</span></div><div>Цена: '+money(item.price_per_kg_minor||0,item.currency||currency)+'/кг</div>'+provenance+weights+'<div class="muted">'+esc(dateLabel(item.timestamp))+'</div>'+(item.comment?'<div>'+esc(item.comment)+'</div>':'')+'</div>';
}
function updateHistoryPager(count,total){
    if($('historyPrev'))$('historyPrev').disabled=historyStack.length===0;
    if($('historyNext'))$('historyNext').disabled=!historyHasMore;
    if($('historyPageInfo'))$('historyPageInfo').textContent='История · страница '+historyPage+' · '+count+' из '+total;
}
async function loadHistory(cursor=historyCursor,reset=false){
    if(reset){historyStack=[];historyPage=1;cursor=0}
    try{
        const q=new URLSearchParams({repair_id:repairId,limit:'20'});if(cursor)q.set('cursor',String(cursor));
        const data=await jsonFetch('/api/warehouse/write-offs?'+q,{cache:'no-store'});
        historyCursor=cursor;historyHasMore=data.has_more===true;historyNextCursor=historyHasMore?Number(data.next_cursor):0;
        if(historyHasMore&&(!Number.isSafeInteger(historyNextCursor)||historyNextCursor<=historyCursor))throw new Error('invalid_history_cursor');
        const items=Array.isArray(data.items)?data.items:[];
        const currency=(items.find(item=>item&&item.currency)||{}).currency||'KGS';
        if($('historyTotal'))$('historyTotal').textContent=kgFromGrams(data.total_consumed_g||0);
        if($('historyValue'))$('historyValue').textContent=money(data.total_consumed_value_minor||0,currency);
        renderBreakdown(data.material_totals,currency);
        if($('totalsSource')){$('totalsSource').className='source muted';$('totalsSource').textContent='Источник массы и стоимости: сервер ESP32.'}
        if($('history')){$('history').className='';$('history').innerHTML=items.length?items.slice().reverse().map(item=>renderHistoryItem(item,currency)).join(''):'Подтверждённых списаний на этой странице нет.'}
        updateHistoryPager(items.length,Number(data.total_count||items.length));
    }catch(error){
        if($('history')){$('history').className='bad';$('history').textContent='Не удалось загрузить историю: '+error.message}
        if($('historyPrev'))$('historyPrev').disabled=true;
        if($('historyNext'))$('historyNext').disabled=true;
    }
}

if($('historyPrev'))$('historyPrev').onclick=()=>{if(!historyStack.length)return;const cursor=historyStack.pop();historyPage=Math.max(1,historyPage-1);loadHistory(cursor,false)};
if($('historyNext'))$('historyNext').onclick=()=>{if(!historyHasMore)return;historyStack.push(historyCursor);historyPage+=1;loadHistory(historyNextCursor,false)};
if($('form'))$('form').onsubmit=async event=>{
    event.preventDefault();
    if(!writable){setResult('Ремонт закрыт или его состояние не подтверждено. Списание запрещено.','warning');return}
    if(!validId(sourceSessionId)||!validId(sourceRunId)||!selection){setResult('Не найден exact завершённый проход для списания.','bad');return}
    if(!activeSpool||String(activeSpool.spool_id)!==String(selection.spool_id)){setResult('Immutable бухта source session недоступна. Списание заблокировано.','bad');return}
    if(!spoolBridge||String(spoolBridge.spool_id)!==String(activeSpool.spool_id)||!validId(spoolBridge.warehouse_item_id)){setResult('Нет exact bridge бухты к MaterialLedger. Списание заблокировано.','bad');return}
    if(!validId(selectedMaterialRequestId)||!materialRequests.some(item=>String(item.material_request_id)===selectedMaterialRequestId)){setResult('Выберите заявку материалов в статусе DRAFT или ISSUED.','bad');return}
    const quantity=canonicalKg($('quantityKg')&&$('quantityKg').value);
    if(!quantity){setResult('Введите количество в кг, до 3 знаков после запятой, например 1,250.','bad');return}
    if(quantity.grams>=Number(activeSpool.current_weight_g)){setResult('Количество должно быть меньше текущего остатка бухты '+kgFromGrams(activeSpool.current_weight_g)+'.','bad');return}

    const body=new URLSearchParams({
        confirmed:'true',
        material_request_id:selectedMaterialRequestId,
        repair_id:repairId,
        warehouse_item_id:String(spoolBridge.warehouse_item_id),
        quantity_milli_units:String(quantity.grams),
        movement_kind:'ISSUE',
        source_kind:'RUN_WIRE',
        unit:'KG',
        created_at:new Date().toISOString(),
        comment:$('comment')?$('comment').value:'',
        source_session_id:sourceSessionId,
        source_run_id:sourceRunId,
        spool_id:String(activeSpool.spool_id),
        wire_diameter_hundredths_mm:String(activeSpool.diameter_hundredths_mm),
        material_class:String(activeSpool.material_class)
    });

    if($('submitButton'))$('submitButton').disabled=true;
    if($('materialRequestId'))$('materialRequestId').disabled=true;
    setResult('Сохранение атомарного RUN_WIRE списания…','muted');
    try{
        const data=await jsonFetch('/api/material-requests/warehouse',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body});
        if(!validId(data.movement_id)||String(data.spool_id)!==String(activeSpool.spool_id))throw new Error('run_wire_response_identity_mismatch');
        setResult('RUN_WIRE: списано '+kgFromGrams(quantity.grams)+' на '+money(data.cost_amount_minor||0,data.currency||'KGS')+' · заявка №'+selectedMaterialRequestId+' · бухта №'+data.spool_id+' · движение №'+data.movement_id+'. Остаток бухты '+kgFromGrams(data.spool_weight_after_g)+'.','ok');
        if($('quantityKg'))$('quantityKg').value='';
        if($('comment'))$('comment').value='';
        await loadHistory(0,true);
        await prepareNextRun();
    }catch(error){
        setResult('Ошибка: '+error.message,'bad');
    }finally{
        refreshSubmitState();
    }
};

(async()=>{
    setFormEnabled(false);
    resetMaterialRequestContext('Проверка заявок и связи бухты со складом…');
    await loadLifecycle();
    await loadHistory(0,true);
    await prepareNextRun();
})();
})();