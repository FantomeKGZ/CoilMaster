(function(){
  'use strict';

  const CAL_URL='/api/hardware/hall/calibration';
  const HISTORY_URL=CAL_URL+'/history';
  const sleep=ms=>new Promise(r=>setTimeout(r,ms));

  async function request(url,opt={}){
    const response=await fetch(url,{cache:'no-store',...opt});
    let json={};
    try{json=await response.json()}catch(e){}
    if(!response.ok)throw new Error(json.error||('HTTP '+response.status));
    return json;
  }

  function post(url,data){
    return request(url,{
      method:'POST',
      headers:{'Content-Type':'application/x-www-form-urlencoded'},
      body:new URLSearchParams(data||{})
    });
  }

  function stateText(state){
    switch(state){
      case 'WAITING_LOCAL_CONFIRM': return 'Подготовка локального запуска на Arduino';
      case 'ARMED_WAITING_START': return 'Ожидание A или физической START';
      case 'RUNNING': return 'Калибровка выполняется';
      case 'COMPLETED': return 'Калибровка завершена';
      case 'WAITING_APPLY_CONFIRM': return 'Ожидание # для сохранения на Arduino';
      case 'ABORTED': return 'Калибровка прервана';
      case 'IDLE': return 'Готово';
      default: return state||'Нет данных';
    }
  }

  function resultFromState(state){
    if(!state||!state.result_available)return null;
    return {
      available:true,
      valid:!!state.recommendation_valid,
      measurement_id:state.measurement_id,
      baseline:state.baseline,
      min:state.min,
      max:state.max,
      recommended_threshold:state.recommended_threshold,
      recommended_hysteresis:state.recommended_hysteresis,
      direction:state.recommended_direction,
      samples:state.samples,
      duration_ms:state.duration_ms,
      received_at_ms:state.result_received_at_ms
    };
  }

  function resultLabel(value){
    const labels={
      NONE:'Не применялось',APPLIED:'Сохранено',BUSY:'Arduino занята',
      INVALID:'Некорректно',IDENTITY_MISMATCH:'Несовпадение измерения',
      CANCELLED:'Отменено',PERSISTENCE_FAILED:'Ошибка EEPROM',
      UNSUPPORTED:'Не поддерживается',TIMED_OUT:'Нет подтверждения'
    };
    return labels[value]||value||'—';
  }

  function historyCard(){
    let card=document.getElementById('hallCalibrationHistoryCard');
    if(card)return card;
    const main=document.querySelector('main');
    if(!main)return null;
    card=document.createElement('section');
    card.id='hallCalibrationHistoryCard';
    card.className='card';
    card.innerHTML='<h2 class="hall-history-title">Последние калибровки</h2><p class="muted">На microSD ESP32 хранится не более 10 последних результатов. Рекомендация ESP32 и реально сохранённый профиль Arduino показаны отдельно.</p><div id="hallCalibrationHistory" class="note">Загрузка истории…</div>';
    main.appendChild(card);
    return card;
  }

  function makeHistoryRow(item,index){
    const wrapper=document.createElement('div');
    wrapper.style.padding='10px 0';
    wrapper.style.borderBottom=index===0?'0':'1px solid #e8edf1';

    const title=document.createElement('div');
    const strong=document.createElement('b');
    strong.textContent='Измерение #'+String(item.measurement_id||'—');
    title.appendChild(strong);
    const status=document.createElement('span');
    status.textContent=' · '+resultLabel(item.apply_result);
    title.appendChild(status);
    wrapper.appendChild(title);

    const measurement=document.createElement('div');
    measurement.className='muted';
    measurement.textContent='ADC: baseline '+item.baseline+' · min '+item.min+' · max '+item.max+' · span '+item.span+' · samples '+item.samples+' · '+item.duration_ms+' мс';
    wrapper.appendChild(measurement);

    const recommendation=document.createElement('div');
    recommendation.className='muted';
    recommendation.textContent=item.recommendation_valid
      ? 'Рекомендация ESP32: threshold '+item.recommended_threshold+' · hysteresis '+item.recommended_hysteresis+' · '+item.recommended_direction
      : 'Рекомендация ESP32: недействительна';
    wrapper.appendChild(recommendation);

    const persisted=document.createElement('div');
    persisted.className='muted';
    persisted.textContent=item.persisted_valid
      ? 'Сохранено Arduino EEPROM: threshold '+item.persisted_threshold+' · hysteresis '+item.persisted_hysteresis+' · debounce '+item.persisted_release_debounce_ms+' мс · '+item.persisted_direction
      : 'Arduino EEPROM: профиль не подтверждён';
    wrapper.appendChild(persisted);
    return wrapper;
  }

  async function loadHistory(){
    historyCard();
    const box=document.getElementById('hallCalibrationHistory');
    if(!box)return;
    try{
      const data=await request(HISTORY_URL);
      box.className='note';
      box.textContent='';
      const items=Array.isArray(data.items)?data.items:[];
      if(!items.length){
        box.textContent='История пока пуста.';
        return;
      }
      for(let i=0;i<items.length;i++)box.appendChild(makeHistoryRow(items[i],i));
    }catch(e){
      box.className='note bad';
      box.textContent='История недоступна: '+e.message;
    }
  }

  function mount(options={}){
    const get=id=>document.getElementById(id);
    const box=get(options.statusId||'calibrationStatus');
    const resultBox=get(options.resultId||'calibrationResult');
    const armBtn=get(options.armButtonId||'calibrationArmBtn');
    const abortBtn=get(options.abortButtonId||'calibrationAbortBtn');
    const applyBtn=get(options.applyButtonId||'calibrationApplyBtn');
    const thresholdInput=get(options.thresholdId||'threshold');
    const hysteresisInput=get(options.hysteresisId||'hysteresis');
    const directionInput=get(options.directionId||'direction');
    const debounceInput=get(options.debounceId||'debounce');
    if(!box||!resultBox||!armBtn||!abortBtn||!applyBtn)return;

    historyCard();
    loadHistory();

    let polling=false;
    let pollTimer=null;
    let lastResult=null;

    function setStatus(text,kind='note'){
      box.className=kind;
      box.textContent=text;
    }

    function renderResult(result){
      lastResult=result&&result.available?result:null;
      if(!lastResult){
        resultBox.className='note';
        resultBox.textContent='Результат калибровки ещё не получен.';
        applyBtn.disabled=true;
        return;
      }
      const valid=!!lastResult.valid&&Number(lastResult.measurement_id)>0;
      resultBox.className=valid?'note ok':'note warn';
      resultBox.innerHTML='Результат: <b>'+(valid?'VALID':'INVALID')+'</b>'+
        ' · baseline '+lastResult.baseline+
        ' · min '+lastResult.min+
        ' · max '+lastResult.max+
        ' · threshold '+lastResult.recommended_threshold+
        ' · hysteresis '+lastResult.recommended_hysteresis+
        ' · '+lastResult.direction+
        ' · samples '+lastResult.samples+
        ' · '+lastResult.duration_ms+' мс';
      applyBtn.disabled=!valid;
    }

    function render(state){
      const name=state.state||'IDLE';
      const pending=!!state.pending;
      if(name==='WAITING_LOCAL_CONFIRM'){
        setStatus('Команда принята. Arduino автоматически готовит локальный запуск и собирает baseline при остановленном двигателе. Дополнительное подтверждение # не требуется.','note warn');
      }else if(name==='ARMED_WAITING_START'){
        setStatus('Дождитесь готовности baseline и запустите тест клавишей A на клавиатуре или отдельной физической START на станке. ESP32 не запускает двигатель.','note warn');
      }else if(name==='RUNNING'){
        setStatus('Калибровка выполняется 15 секунд. Любая клавиша или повторная START на станке прервёт процедуру.','note ok');
      }else if(name==='COMPLETED'){
        setStatus('Калибровка завершена. Проверьте рекомендацию. Кнопка применения только отправит proposal; EEPROM изменится лишь после отдельного # на Arduino.','note ok');
      }else if(name==='WAITING_APPLY_CONFIRM'){
        setStatus('Proposal принят Arduino. Для записи параметров в EEPROM нажмите # на Arduino. START не подтверждает сохранение. Другая клавиша или потеря связи отменит применение.','note warn');
      }else if(name==='ABORTED'){
        setStatus('Калибровка или применение прервано. Двигатель должен оставаться остановленным.','note warn');
      }else{
        setStatus('Автокалибровка: '+stateText(name)+(pending?' · команда выполняется':''),pending?'note warn':'note');
      }
      const active=name==='WAITING_LOCAL_CONFIRM'||name==='ARMED_WAITING_START'||
        name==='RUNNING'||name==='WAITING_APPLY_CONFIRM';
      armBtn.disabled=pending||active;
      abortBtn.disabled=!active;
      renderResult(resultFromState(state));
      if(name==='WAITING_APPLY_CONFIRM')applyBtn.disabled=true;
    }

    async function load(refresh=false){
      if(refresh){
        await post(CAL_URL+'/refresh');
        await sleep(250);
      }
      const state=await request(CAL_URL);
      render(state);
      return state;
    }

    async function poll(){
      if(!polling)return;
      try{
        const state=await load(false);
        const terminal=!state.pending&&
          (state.state==='COMPLETED'||state.state==='ABORTED'||state.state==='IDLE');
        if(terminal){
          polling=false;
          if(state.last_reply==='APPLIED'){
            setStatus('Параметры подтверждены на Arduino и сохранены в EEPROM.','note ok');
            document.dispatchEvent(new CustomEvent('cm-hall-settings-applied'));
          }else if(state.last_reply&&state.last_reply!=='NONE'){
            setStatus('Применение завершилось: '+state.last_reply,'note warn');
          }
          await loadHistory();
          return;
        }
      }catch(e){
        setStatus('Ошибка автокалибровки: '+e.message,'note bad');
      }
      pollTimer=setTimeout(poll,500);
    }

    armBtn.addEventListener('click',async()=>{
      armBtn.disabled=true;
      try{
        setStatus('Подготовка автокалибровки…','note warn');
        await post(CAL_URL+'/arm');
        polling=true;
        clearTimeout(pollTimer);
        setTimeout(poll,250);
      }catch(e){
        armBtn.disabled=false;
        setStatus('Не удалось вооружить калибровку: '+e.message,'note bad');
      }
    });

    abortBtn.addEventListener('click',async()=>{
      abortBtn.disabled=true;
      try{
        await post(CAL_URL+'/abort');
        polling=true;
        clearTimeout(pollTimer);
        setTimeout(poll,200);
      }catch(e){
        setStatus('Не удалось прервать калибровку: '+e.message,'note bad');
        load(false).catch(()=>{abortBtn.disabled=false;});
      }
    });

    applyBtn.addEventListener('click',async()=>{
      if(!lastResult||!lastResult.valid||Number(lastResult.measurement_id)<=0)return;
      if(!thresholdInput||!hysteresisInput||!directionInput||!debounceInput){
        setStatus('Поля настроек Hall не найдены.','note bad');
        return;
      }
      const debounce=Number(debounceInput.value);
      if(!Number.isInteger(debounce)||debounce<1||debounce>1000){
        setStatus('Перед применением укажите корректную задержку отпускания 1–1000 мс.','note bad');
        return;
      }
      thresholdInput.value=lastResult.recommended_threshold;
      hysteresisInput.value=lastResult.recommended_hysteresis;
      directionInput.value=lastResult.direction;
      applyBtn.disabled=true;
      try{
        setStatus('Отправка exact calibration proposal на Arduino…','note warn');
        await post(CAL_URL+'/apply',{
          release_debounce_ms:String(debounce)
        });
        polling=true;
        clearTimeout(pollTimer);
        setTimeout(poll,200);
      }catch(e){
        applyBtn.disabled=false;
        setStatus('Не удалось подготовить применение: '+e.message,'note bad');
      }
    });

    document.addEventListener('visibilitychange',()=>{
      if(document.hidden&&polling){
        polling=false;
        clearTimeout(pollTimer);
      }
    });
    window.addEventListener('pagehide',()=>{
      polling=false;
      clearTimeout(pollTimer);
    });

    load(true).catch(e=>setStatus('Ошибка автокалибровки: '+e.message,'note bad'));
  }

  window.CMHallCalibration={mount,loadHistory};
})();
