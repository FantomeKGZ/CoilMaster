(function(){
  'use strict';

  const CAL_URL='/api/hardware/hall/calibration';
  const HALL_URL='/api/hardware/hall';
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
      case 'WAITING_LOCAL_CONFIRM': return 'Ожидание подтверждения # на Arduino';
      case 'ARMED_WAITING_START': return 'Ожидание физической START';
      case 'RUNNING': return 'Калибровка выполняется';
      case 'COMPLETED': return 'Калибровка завершена';
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
      const valid=!!lastResult.valid;
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
        setStatus('Команда принята. Подтвердите калибровку клавишей # на Arduino. До подтверждения физическая START не запускает калибровку.','note warn');
      }else if(name==='ARMED_WAITING_START'){
        setStatus('Локальное подтверждение принято. Дождитесь готовности baseline и нажмите отдельную физическую START на станке. ESP32 не запускает двигатель.','note warn');
      }else if(name==='RUNNING'){
        setStatus('Калибровка выполняется. Любая клавиша или повторная START на станке прервёт процедуру.','note ok');
      }else if(name==='COMPLETED'){
        setStatus('Калибровка завершена. Проверьте результат и примените параметры только вручную.','note ok');
      }else if(name==='ABORTED'){
        setStatus('Калибровка прервана. Двигатель должен оставаться остановленным.','note warn');
      }else{
        setStatus('Автокалибровка: '+stateText(name)+(pending?' · команда выполняется':''),pending?'note warn':'note');
      }
      const active=name==='WAITING_LOCAL_CONFIRM'||name==='ARMED_WAITING_START'||name==='RUNNING';
      armBtn.disabled=pending||active;
      abortBtn.disabled=pending||!active;
      renderResult(resultFromState(state));
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
          return;
        }
      }catch(e){
        setStatus('Ошибка автокалибровки: '+e.message,'note bad');
      }
      pollTimer=setTimeout(poll,500);
    }

    armBtn.addEventListener('click',async()=>{
      try{
        setStatus('Подготовка автокалибровки…','note warn');
        await post(CAL_URL+'/arm');
        polling=true;
        clearTimeout(pollTimer);
        setTimeout(poll,250);
      }catch(e){
        setStatus('Не удалось вооружить калибровку: '+e.message,'note bad');
      }
    });

    abortBtn.addEventListener('click',async()=>{
      try{
        await post(CAL_URL+'/abort');
        polling=true;
        clearTimeout(pollTimer);
        setTimeout(poll,200);
      }catch(e){
        setStatus('Не удалось прервать калибровку: '+e.message,'note bad');
      }
    });

    applyBtn.addEventListener('click',async()=>{
      if(!lastResult||!lastResult.valid)return;
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
      try{
        setStatus('Применение рекомендованных параметров…','note warn');
        await post(HALL_URL+'/settings',{
          threshold:String(lastResult.recommended_threshold),
          hysteresis:String(lastResult.recommended_hysteresis),
          release_debounce_ms:String(debounce),
          direction:lastResult.direction
        });
        let applied=false;
        for(let i=0;i<12;i++){
          await sleep(250);
          const hall=await request(HALL_URL);
          if(!hall.pending&&hall.last_reply&&hall.last_reply!=='NONE'){
            if(hall.last_reply!=='APPLIED')throw new Error(hall.last_reply);
            applied=true;
            break;
          }
        }
        if(!applied)throw new Error('timeout');
        setStatus('Рекомендованные threshold / hysteresis / direction применены вручную.','note ok');
        document.dispatchEvent(new CustomEvent('cm-hall-settings-applied'));
      }catch(e){
        setStatus('Не удалось применить параметры: '+e.message,'note bad');
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

  window.CMHallCalibration={mount};
})();
