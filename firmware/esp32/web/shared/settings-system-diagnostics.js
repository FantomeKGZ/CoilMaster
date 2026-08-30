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
        EXTERNAL_PIN:'внешний сброс',
        SOFTWARE:'программная перезагрузка',
        PANIC:'авария прошивки',
        INTERRUPT_WATCHDOG:'сторожевой таймер прерывания',
        TASK_WATCHDOG:'сторожевой таймер задачи',
        WATCHDOG:'сторожевой таймер',
        DEEP_SLEEP:'выход из глубокого сна',
        BROWNOUT:'просадка питания',
        SDIO:'сброс SDIO',
        UNKNOWN:'причина не определена'
    };
    const heapSize=value=>Math.round(Number(value||0)/1024)+' КБ';
    const byteSize=value=>{
        const number=Number(value);
        if(!Number.isFinite(number)||number<0)return '—';
        const units=['Б','КБ','МБ','ГБ','ТБ'];
        let scaled=number,index=0;
        while(scaled>=1024&&index<units.length-1){scaled/=1024;index++;}
        return scaled.toLocaleString('ru-RU',{
            maximumFractionDigits:index===0?0:2
        })+' '+units[index];
    };
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
    async function loadStorage(){
        try{
            const response=await fetch('/api/system/storage',{cache:'no-store'});
            const data=await response.json();
            if(!response.ok)throw new Error(data.error||'storage_diagnostics_unavailable');
            return data;
        }catch(error){
            return null;
        }
    }
    async function load(){
        root.className='note';
        root.textContent='Загрузка диагностики ESP32…';
        try{
            const storagePromise=loadStorage();
            const response=await fetch('/api/system/diagnostics',{cache:'no-store'});
            const data=await response.json();
            if(!response.ok)throw new Error(data.error||'diagnostics_unavailable');
            const storage=await storagePromise;
            root.textContent='';
            addRow('Причина прошлого сброса',labels[data.reset_reason]||String(data.reset_reason||'—'));
            addRow('Свободная динамическая память',heapSize(data.free_heap_bytes));
            addRow('Минимум свободной памяти после запуска',heapSize(data.minimum_free_heap_bytes));
            addRow('Крупнейший свободный блок памяти',heapSize(data.largest_free_heap_block_bytes));

            if(storage){
                addRow('microSD',storage.storage_ready?'готова':'не готова');
                addRow('Физический размер карты',byteSize(storage.card_size_bytes));
                const total=Number(storage.filesystem_total_bytes||0);
                const used=Number(storage.filesystem_used_bytes||0);
                const free=Number(storage.filesystem_free_bytes||0);
                addRow('Файловая система',byteSize(used)+' / '+byteSize(total));
                const freePercent=total>0?Math.max(0,Math.min(100,free*100/total)):null;
                addRow('Свободно на microSD',byteSize(free)+(freePercent===null?'':' ('+freePercent.toLocaleString('ru-RU',{maximumFractionDigits:1})+'%)'));
                addRow('Журнал списаний',byteSize(storage.warehouse_movements_bytes));
                addRow('Журнал намоток',byteSize(storage.winding_events_bytes));
                addRow('Реестр ремонтов',byteSize(storage.repair_registry_bytes));
                addRow('Реестр бухт',byteSize(storage.wire_spools_bytes));
            }else{
                addRow('microSD','диагностика недоступна');
            }

            const note=document.createElement('div');
            const storageFailed=storage&&storage.storage_ready!==true;
            if(data.brownout_reset_detected===true){
                root.className='note warn';
                note.textContent='Зафиксирована просадка питания: проверьте источник 5 В, кабель, общую землю и напряжение 3,3 В во время старта Wi‑Fi.';
            }else if(storageFailed){
                root.className='note warn';
                note.textContent='microSD не готова: проверьте карту и перезагрузите устройство. Автоматическое удаление рабочих данных не выполняется.';
            }else{
                note.className='muted';
                note.textContent='Просадка питания прошлого запуска не зафиксирована. Размеры растущих NDJSON-журналов показаны для наблюдения; автоматическая очистка и ротация рабочих данных отключены.';
            }
            root.appendChild(note);
        }catch(error){
            root.className='note bad';
            root.textContent='Диагностика ESP32 недоступна.';
        }
    }
    load();
})();