(function(){
    'use strict';
    const network=document.getElementById('networkSummary');
    if(!network||document.getElementById('systemDiagnostics'))return;
    const root=document.createElement('div');
    root.id='systemDiagnostics';
    root.className='note';
    root.textContent='Загрузка диагностики ESP32…';
    network.insertAdjacentElement('afterend',root);

    const labels={
        POWER_ON:'включение питания',
        EXTERNAL_PIN:'внешний RESET',
        SOFTWARE:'программная перезагрузка',
        PANIC:'авария прошивки',
        INTERRUPT_WATCHDOG:'watchdog прерывания',
        TASK_WATCHDOG:'watchdog задачи',
        WATCHDOG:'watchdog',
        DEEP_SLEEP:'выход из deep sleep',
        BROWNOUT:'просадка питания (brownout)',
        SDIO:'сброс SDIO',
        UNKNOWN:'причина не определена'
    };
    const size=value=>Math.round(Number(value||0)/1024)+' КБ';
    function addRow(label,value){
        const row=document.createElement('div');
        row.className='row';
        const name=document.createElement('span');
        name.textContent=label;
        const result=document.createElement('b');
        result.textContent=value;
        row.append(name,result);
        root.appendChild(row);
    }
    async function load(){
        root.className='note';
        root.textContent='Загрузка диагностики ESP32…';
        try{
            const response=await fetch('/api/system/diagnostics',{cache:'no-store'});
            const data=await response.json();
            if(!response.ok)throw new Error(data.error||'diagnostics_unavailable');
            root.textContent='';
            addRow('Причина прошлого сброса',labels[data.reset_reason]||String(data.reset_reason||'—'));
            addRow('Свободная heap',size(data.free_heap_bytes));
            addRow('Минимальная heap после запуска',size(data.minimum_free_heap_bytes));
            addRow('Крупнейший свободный блок',size(data.largest_free_heap_block_bytes));
            const note=document.createElement('div');
            if(data.brownout_reset_detected===true){
                root.className='note warn';
                note.textContent='Зафиксирован brownout: проверьте источник 5 В, кабель, общую землю и напряжение 3,3 В во время старта Wi‑Fi.';
            }else{
                note.className='muted';
                note.textContent='Brownout прошлого запуска не зафиксирован. Это не заменяет измерение 5 В и 3,3 В мультиметром под нагрузкой.';
            }
            root.appendChild(note);
        }catch(error){
            root.className='note bad';
            root.textContent='Диагностика ESP32 недоступна.';
        }
    }
    load();
})();
