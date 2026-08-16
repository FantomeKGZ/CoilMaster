(()=>{
    let timer=0;
    const manifestState=document.getElementById('state');
    if(!manifestState)return;
    const status=document.createElement('div');
    status.id='remoteBackupState';
    status.className='muted';
    status.style.marginTop='10px';
    status.textContent='FTP: ожидание запуска.';
    const full=document.createElement('button');
    full.type='button';
    full.id='remoteBackupBatch';
    full.textContent='Создать полную копию на сервере';
    const retention=document.createElement('button');
    retention.type='button';
    retention.id='remoteBackupRetention';
    retention.textContent='Применить лимит копий сейчас';
    const inspectId=document.createElement('input');
    inspectId.type='number';
    inspectId.id='remoteBackupInspectId';
    inspectId.min='1';
    inspectId.step='1';
    inspectId.placeholder='ID копии';
    inspectId.setAttribute('aria-label','ID резервной копии');
    const inspect=document.createElement('button');
    inspect.type='button';
    inspect.id='remoteBackupInspect';
    inspect.textContent='Проверить копию без восстановления';
    const stage=document.createElement('button');
    stage.type='button';
    stage.id='remoteBackupStage';
    stage.textContent='Загрузить во временную область';
    const plan=document.createElement('button');
    plan.type='button';
    plan.id='remoteBackupRestorePlan';
    plan.textContent='Проверить план восстановления';
    const rollback=document.createElement('button');
    rollback.type='button';
    rollback.id='remoteBackupRollbackSnapshot';
    rollback.textContent='Создать локальную страховочную копию';
    const preflight=document.createElement('button');
    preflight.type='button';
    preflight.id='remoteBackupApplyPreflight';
    preflight.textContent='Проверить готовность к применению';
    const apply=document.createElement('button');
    apply.type='button';
    apply.id='remoteBackupApply';
    apply.textContent='Применить проверенную копию';
    const discard=document.createElement('button');
    discard.type='button';
    discard.id='remoteBackupStageDiscard';
    discard.textContent='Удалить временные файлы';
    manifestState.parentNode.insertBefore(status,manifestState.nextSibling);
    status.insertAdjacentElement('afterend',full);
    full.insertAdjacentElement('afterend',retention);
    retention.insertAdjacentElement('afterend',inspectId);
    inspectId.insertAdjacentElement('afterend',inspect);
    inspect.insertAdjacentElement('afterend',stage);
    stage.insertAdjacentElement('afterend',plan);
    plan.insertAdjacentElement('afterend',rollback);
    rollback.insertAdjacentElement('afterend',preflight);
    preflight.insertAdjacentElement('afterend',apply);
    apply.insertAdjacentElement('afterend',discard);

    const size=n=>{
        const v=Number(n)||0;
        return v<1024?v+' Б':(v/1024).toFixed(1)+' КБ';
    };
    function enhance(){
        document.querySelectorAll('a[href^="/api/backup/file?name="]').forEach(a=>{
            if(a.dataset.remoteReady)return;
            a.dataset.remoteReady='1';
            const name=new URL(a.href,location.href).searchParams.get('name');
            if(!name)return;
            const b=document.createElement('button');
            b.type='button';
            b.dataset.remoteBackup=name;
            b.textContent='На сервер';
            a.insertAdjacentElement('afterend',b);
        });
    }
    function buttons(disabled){
        document.querySelectorAll('[data-remote-backup]').forEach(b=>b.disabled=disabled);
        full.disabled=disabled;
        retention.disabled=disabled;
        inspectId.disabled=disabled;
        inspect.disabled=disabled;
        stage.disabled=disabled;
        plan.disabled=disabled;
        rollback.disabled=disabled;
        preflight.disabled=disabled;
        apply.disabled=disabled;
        discard.disabled=disabled;
    }
    async function pollBatch(){
        enhance();
        try{
            const r=await fetch('/api/backup/remote/batch-status',{cache:'no-store'}),j=await r.json();
            if(!r.ok)throw new Error(j.error||'batch_status_failed');
            if(j.active){
                buttons(true);
                status.className='muted';
                if(j.state==='RETENTION'){
                    status.textContent=j.operation==='RETENTION_ONLY'
                        ?'Применение лимита: удалено файлов '+j.retention_files_deleted+' · незавершённых наборов '+j.incomplete_batches_deleted+'.'
                        :'Полная копия №'+j.batch_id+' завершена. Очистка старых и незавершённых управляемых копий · удалено файлов '+j.retention_files_deleted+'.';
                }else{
                    status.textContent='Полная копия №'+j.batch_id+': '+j.state+' · файлов '+j.files_completed+' · '+size(j.bytes_sent)+' из '+size(j.bytes_total);
                }
                timer=setTimeout(pollBatch,700);
                return;
            }
            buttons(false);
            if(j.state==='COMPLETED'&&j.operation==='RETENTION_ONLY'&&j.retention_succeeded===false){
                status.className='bad';
                status.textContent='Не удалось применить лимит копий: '+(j.retention_error||'unknown')+'. Действующие копии не повреждены.';
            }else if(j.state==='COMPLETED'&&j.operation==='RETENTION_ONLY'){
                status.className='ok';
                status.textContent='Лимит применён. Удалено файлов: '+j.retention_files_deleted+'; незавершённых наборов: '+j.incomplete_batches_deleted+'.';
            }else if(j.state==='COMPLETED'&&j.retention_succeeded===false){
                status.className='warning';
                status.textContent='Полная копия №'+j.batch_id+' завершена и имеет COMPLETE, но очистка старых копий не закончена: '+(j.retention_error||'unknown')+'. Новая копия действительна.';
            }else if(j.state==='COMPLETED'){
                status.className='ok';
                status.textContent='Полная копия №'+j.batch_id+' завершена. Файлов: '+j.files_completed+'. Маркер COMPLETE создан. Ротация выполнена.';
            }else if(j.state==='FAILED'){
                status.className='bad';
                status.textContent='Полная копия №'+j.batch_id+' не завершена: '+(j.error||'unknown')+'. Без COMPLETE этот набор недействителен.';
            }else pollSingle();
        }catch(e){
            buttons(false);
            status.className='bad';
            status.textContent='FTP batch status недоступен: '+e.message;
        }
    }
    async function pollSingle(){
        enhance();
        try{
            const r=await fetch('/api/backup/remote/status',{cache:'no-store'}),j=await r.json();
            if(!r.ok)throw new Error(j.error||'status_failed');
            if(j.active){
                status.className='muted';
                status.textContent='FTP: '+j.state+' · '+size(j.bytes_sent)+' из '+size(j.bytes_total);
                buttons(true);
                timer=setTimeout(pollSingle,700);
                return;
            }
            buttons(false);
            if(j.succeeded){
                status.className='ok';
                status.textContent='FTP: файл '+j.name+' передан, проверен и переименован.';
            }else if(j.state==='FAILED'){
                status.className='bad';
                status.textContent='FTP: ошибка '+(j.error||'unknown')+'. На сервере может остаться только .part.';
            }else status.textContent='FTP: ожидание запуска.';
        }catch(e){
            buttons(false);
            status.className='bad';
            status.textContent='FTP status недоступен: '+e.message;
        }
    }
    async function pollInspection(fallback){
        try{
            const r=await fetch('/api/backup/remote/inspection-status',{cache:'no-store'}),j=await r.json();
            if(!r.ok)throw new Error(j.error||'inspection_status_failed');
            if(j.active){
                buttons(true);
                status.className='muted';
                status.textContent=j.state==='FILES'
                    ?'Проверка копии №'+j.batch_id+': файлов '+j.files_verified+' из '+j.data_files+(j.sizes_verified?' · сверка размеров.':' · проверка наличия (старый манифест V1).')+' Рабочие данные не изменяются.'
                    :'Проверка копии №'+j.batch_id+': '+j.state+' · '+size(j.bytes_received)+' из '+size(j.bytes_total)+'. Рабочие данные не изменяются.';
                timer=setTimeout(()=>pollInspection(false),700);
            }else if(j.state==='VALID'){
                buttons(false);
                status.className='ok';
                status.textContent='Копия №'+j.batch_id+': '+(j.sizes_verified?'наличие и размеры всех файлов подтверждены':'наличие всех файлов подтверждено; размеры недоступны в старом манифесте V1')+', файлов данных '+j.data_files+'. Восстановление не выполнялось.';
            }else if(j.state==='FAILED'){
                buttons(false);
                status.className='bad';
                status.textContent='Копия №'+j.batch_id+' не прошла проверку метаданных: '+(j.error||'unknown')+'. Рабочие данные не изменены.';
            }else if(fallback)pollBatch();
        }catch(e){
            buttons(false);
            status.className='bad';
            status.textContent='Статус проверки копии недоступен: '+e.message;
        }
    }
    async function pollStaging(fallback){
        try{
            const r=await fetch('/api/backup/remote/staging-status',{cache:'no-store'}),j=await r.json();
            if(!r.ok)throw new Error(j.error||'staging_status_failed');
            if(j.active){
                buttons(true);
                status.className='muted';
                status.textContent='Временная загрузка копии №'+j.batch_id+': файлов '+j.files_completed+' из '+j.files_total+' · '+size(j.bytes_completed)+' из '+size(j.bytes_total)+'. Рабочие данные не изменяются.';
                timer=setTimeout(()=>pollStaging(false),700);
            }else if(j.state==='STAGED'){
                buttons(false);
                status.className='ok';
                status.textContent='Копия №'+j.batch_id+' загружена во временную область: '+j.files_completed+' файлов, '+size(j.bytes_completed)+'. Восстановление отключено и не выполнялось.';
            }else if(j.state==='FAILED'){
                buttons(false);
                status.className='bad';
                status.textContent='Временная загрузка не завершена: '+(j.error||'unknown')+'. Неполный временный набор удалён; рабочие данные не изменены.';
            }else if(j.state==='STALE'){
                buttons(false);
                status.className='warning';
                status.textContent='Найдена временная область после перезапуска. Автовосстановление запрещено; удалите временные файлы перед новой проверкой.';
            }else if(fallback)pollInspection(true);
        }catch(e){
            buttons(false);
            status.className='bad';
            status.textContent='Статус временной загрузки недоступен: '+e.message;
        }
    }
    async function pollRestorePlan(fallback){
        try{
            const r=await fetch('/api/backup/remote/restore-plan-status',{cache:'no-store'}),j=await r.json();
            if(!r.ok)throw new Error(j.error||'restore_plan_status_failed');
            if(j.active){
                buttons(true);
                status.className='muted';
                status.textContent='Проверка плана копии №'+j.batch_id+': файлов '+j.files_planned+' из '+j.files_total+' · '+size(j.bytes_planned)+' из '+size(j.bytes_total)+'. Рабочие данные не изменяются.';
                timer=setTimeout(()=>pollRestorePlan(false),300);
            }else if(j.state==='VALID'){
                buttons(false);
                status.className='ok';
                status.textContent='План копии №'+j.batch_id+' проверен: '+j.files_planned+' файлов сопоставлены только с разрешёнными путями, размеры подтверждены. Применение данных отключено.';
            }else if(j.state==='FAILED'){
                buttons(false);
                status.className='bad';
                status.textContent='План восстановления отклонён: '+(j.error||'unknown')+'. Рабочие данные не изменены.';
            }else if(fallback)pollStaging(true);
        }catch(e){
            buttons(false);
            status.className='bad';
            status.textContent='Статус плана восстановления недоступен: '+e.message;
        }
    }
    async function pollRollback(fallback){
        try{
            const r=await fetch('/api/backup/remote/rollback-snapshot-status',{cache:'no-store'}),j=await r.json();
            if(!r.ok)throw new Error(j.error||'rollback_status_failed');
            if(j.active){
                buttons(true);
                status.className='muted';
                status.textContent='Локальная страховочная копия №'+j.batch_id+': '+j.state+' · файлов '+j.files_processed+' из '+j.files_total+' · сохранено '+size(j.bytes_copied)+(j.current_total?' · текущий файл '+size(j.current_bytes)+' из '+size(j.current_total):'')+'. Рабочие данные не изменяются.';
                timer=setTimeout(()=>pollRollback(false),300);
            }else if(j.state==='READY'){
                buttons(false);
                status.className='ok';
                status.textContent='Страховочная копия готова и проверена CRC32: сохранено файлов '+j.files_present+', отсутствующих рабочих файлов '+j.files_missing+', '+size(j.bytes_copied)+'. Применение восстановления отключено.';
            }else if(j.state==='FAILED'){
                buttons(false);
                status.className='bad';
                status.textContent='Страховочная копия не создана: '+(j.error||'unknown')+'. Неполный набор удалён, рабочие данные не изменены.';
            }else if(j.state==='STALE'){
                buttons(false);
                status.className='warning';
                status.textContent='Найдена страховочная область после перезапуска. Автоприменение запрещено; удалите временные файлы перед новым циклом.';
            }else if(fallback)pollRestorePlan(true);
        }catch(e){
            buttons(false);
            status.className='bad';
            status.textContent='Статус страховочной копии недоступен: '+e.message;
        }
    }
    async function pollApplyPreflight(fallback){
        try{
            const r=await fetch('/api/backup/remote/apply-preflight-status',{cache:'no-store'}),j=await r.json();
            if(!r.ok)throw new Error(j.error||'apply_preflight_status_failed');
            if(j.active){
                buttons(true);
                status.className='muted';
                status.textContent='Контроль готовности копии №'+j.batch_id+': '+j.state+' · файлов '+j.files_checked+' из '+j.files_total+' · проверено данных '+size(j.staged_bytes_checked)+' из '+size(j.staged_bytes_total)+(j.current_total?' · текущая проверка '+size(j.current_bytes)+' из '+size(j.current_total):'')+'. Рабочие данные не изменяются.';
                timer=setTimeout(()=>pollApplyPreflight(false),300);
            }else if(j.state==='READY'){
                buttons(false);
                status.className='ok';
                status.textContent='Готовность копии №'+j.batch_id+' подтверждена: текущие рабочие файлы совпадают со страховочной копией, CRC32 временных файлов записаны. Применение восстановления всё ещё отключено.';
            }else if(j.state==='FAILED'){
                buttons(false);
                status.className='bad';
                status.textContent='Контроль готовности отклонён: '+(j.error||'unknown')+'. Временная и страховочная копии сохранены, рабочие данные не изменены.';
            }else if(j.state==='STALE'){
                buttons(false);
                status.className='warning';
                status.textContent='Найдены результаты контроля после перезапуска. Автоприменение запрещено; удалите временные файлы перед новым циклом.';
            }else if(fallback)pollRollback(true);
        }catch(e){
            buttons(false);
            status.className='bad';
            status.textContent='Статус контроля готовности недоступен: '+e.message;
        }
    }
    async function pollApply(fallback){
        try{
            const r=await fetch('/api/backup/remote/apply-status',{cache:'no-store'}),j=await r.json();
            if(!r.ok)throw new Error(j.error||'apply_status_failed');
            if(j.active){
                buttons(true);
                status.className=j.state==='ROLLING_BACK'?'warning':'muted';
                status.textContent=j.state==='ROLLING_BACK'
                    ?'Ошибка применения копии №'+j.batch_id+'. Выполняется немедленный откат: восстановлено файлов '+j.files_restored+'.'
                    :'Применение копии №'+j.batch_id+': '+j.state+' · файлов '+j.files_applied+' из '+j.files_total+' · '+size(j.bytes_applied)+(j.current_total?' · текущий файл '+size(j.current_bytes)+' из '+size(j.current_total):'')+'. Не выключайте питание.';
                timer=setTimeout(()=>pollApply(false),300);
            }else if(j.state==='APPLIED'){
                buttons(true);
                status.className='ok';
                status.textContent='Копия №'+j.batch_id+' применена: '+j.files_applied+' файлов. Перезагрузите ESP32 перед дальнейшей работой; автоматическое продолжение восстановления после reboot отсутствует.';
            }else if(j.state==='ROLLED_BACK'){
                buttons(false);
                status.className='warning';
                status.textContent='Применение остановлено, исходные рабочие файлы восстановлены: '+j.files_restored+'. Причина: '+(j.rollback_reason||'unknown')+'. Удалите временные файлы перед новой попыткой.';
            }else if(j.state==='FAILED'){
                buttons(true);
                discard.disabled=false;
                status.className='bad';
                status.textContent='Критическая ошибка применения/отката: '+(j.error||'unknown')+'. Не продолжайте работу со станком; сохраните microSD и проверьте страховочную область.';
            }else if(j.state==='STALE'){
                buttons(false);
                status.className='warning';
                status.textContent='Найдены следы предыдущего применения после перезапуска. Автопродолжение запрещено. Проверьте данные и удалите временные файлы только после подтверждения состояния.';
            }else if(fallback)pollApplyPreflight(true);
        }catch(e){
            buttons(false);
            status.className='bad';
            status.textContent='Статус применения недоступен: '+e.message;
        }
    }
    async function upload(name){
        clearTimeout(timer);
        buttons(true);
        status.className='muted';
        status.textContent='FTP: запуск передачи…';
        try{
            const body=new URLSearchParams({name});
            const r=await fetch('/api/backup/remote/upload',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body}),j=await r.json();
            if(!r.ok)throw new Error(j.reason||j.error||'start_failed');
            pollSingle();
        }catch(e){
            buttons(false);
            status.className='bad';
            status.textContent='FTP: запуск отклонён — '+e.message;
        }
    }
    async function batch(){
        if(!confirm('Создать полный набор основных данных и всех файлов сессий? Во время копирования не изменяйте данные.'))return;
        clearTimeout(timer);
        buttons(true);
        status.className='muted';
        status.textContent='Проверка и запуск полной копии…';
        try{
            const r=await fetch('/api/backup/remote/batch',{method:'POST'}),j=await r.json();
            if(!r.ok)throw new Error(j.error||'batch_start_failed');
            pollBatch();
        }catch(e){
            buttons(false);
            status.className='bad';
            status.textContent='Полная копия не запущена: '+e.message;
        }
    }
    async function applyRetention(){
        if(!confirm('Применить текущий лимит и удалить самые старые управляемые копии?'))return;
        clearTimeout(timer);
        buttons(true);
        status.className='muted';
        status.textContent='Проверка и применение лимита копий…';
        try{
            const r=await fetch('/api/backup/remote/retention',{method:'POST'}),j=await r.json();
            if(!r.ok)throw new Error(j.error||'retention_start_failed');
            pollBatch();
        }catch(e){
            buttons(false);
            status.className='bad';
            status.textContent='Лимит копий не применён: '+e.message;
        }
    }
    async function inspectBatch(){
        const batchId=String(inspectId.value||'').trim();
        if(!/^[1-9]\d*$/.test(batchId)){
            status.className='bad';
            status.textContent='Укажите положительный ID копии из имени cm-b<ID>-MANIFEST.txt.';
            return;
        }
        clearTimeout(timer);
        buttons(true);
        status.className='muted';
        status.textContent='Загрузка и проверка метаданных копии №'+batchId+'…';
        try{
            const body=new URLSearchParams({batch_id:batchId});
            const r=await fetch('/api/backup/remote/inspection',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body}),j=await r.json();
            if(!r.ok)throw new Error(j.error||'inspection_start_failed');
            pollInspection(false);
        }catch(e){
            buttons(false);
            status.className='bad';
            status.textContent='Проверка копии не запущена: '+e.message;
        }
    }
    async function stageBatch(){
        const batchId=String(inspectId.value||'').trim();
        if(!/^[1-9]\d*$/.test(batchId)){
            status.className='bad';
            status.textContent='Сначала укажите и успешно проверьте ID новой копии V2.';
            return;
        }
        if(!confirm('Загрузить все файлы проверенной копии во временную область microSD? Рабочие данные заменены не будут.'))return;
        clearTimeout(timer);
        buttons(true);
        status.className='muted';
        status.textContent='Запуск временной загрузки копии №'+batchId+'…';
        try{
            const body=new URLSearchParams({batch_id:batchId});
            const r=await fetch('/api/backup/remote/staging',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body}),j=await r.json();
            if(!r.ok)throw new Error(j.error||'staging_start_failed');
            pollStaging(false);
        }catch(e){
            buttons(false);
            status.className='bad';
            status.textContent='Временная загрузка не запущена: '+e.message;
        }
    }
    async function buildRestorePlan(){
        const batchId=String(inspectId.value||'').trim();
        if(!/^[1-9]\d*$/.test(batchId)){
            status.className='bad';
            status.textContent='Сначала укажите ID загруженной во временную область копии.';
            return;
        }
        clearTimeout(timer);
        buttons(true);
        status.className='muted';
        status.textContent='Формирование строгого плана восстановления копии №'+batchId+'…';
        try{
            const body=new URLSearchParams({batch_id:batchId});
            const r=await fetch('/api/backup/remote/restore-plan',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body}),j=await r.json();
            if(!r.ok)throw new Error(j.error||'restore_plan_start_failed');
            pollRestorePlan(false);
        }catch(e){
            buttons(false);
            status.className='bad';
            status.textContent='План восстановления не создан: '+e.message;
        }
    }
    async function buildRollbackSnapshot(){
        const batchId=String(inspectId.value||'').trim();
        if(!/^[1-9]\d*$/.test(batchId)){
            status.className='bad';
            status.textContent='Сначала укажите ID копии и успешно проверьте план восстановления.';
            return;
        }
        if(!confirm('Создать на microSD отдельную проверяемую копию текущих рабочих файлов? Данные заменены не будут, но потребуется дополнительное место.'))return;
        clearTimeout(timer);
        buttons(true);
        status.className='muted';
        status.textContent='Запуск локальной страховочной копии для набора №'+batchId+'…';
        try{
            const body=new URLSearchParams({batch_id:batchId});
            const r=await fetch('/api/backup/remote/rollback-snapshot',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body}),j=await r.json();
            if(!r.ok)throw new Error(j.error||'rollback_start_failed');
            pollRollback(false);
        }catch(e){
            buttons(false);
            status.className='bad';
            status.textContent='Страховочная копия не запущена: '+e.message;
        }
    }
    async function runApplyPreflight(){
        const batchId=String(inspectId.value||'').trim();
        if(!/^[1-9]\d*$/.test(batchId)){
            status.className='bad';
            status.textContent='Сначала укажите ID копии и создайте страховочную копию.';
            return;
        }
        if(!confirm('Повторно проверить рабочие, страховочные и временные файлы перед будущим применением? Никакие рабочие данные заменены не будут.'))return;
        clearTimeout(timer);
        buttons(true);
        status.className='muted';
        status.textContent='Запуск контроля готовности копии №'+batchId+'…';
        try{
            const body=new URLSearchParams({batch_id:batchId});
            const r=await fetch('/api/backup/remote/apply-preflight',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body}),j=await r.json();
            if(!r.ok)throw new Error(j.error||'apply_preflight_start_failed');
            pollApplyPreflight(false);
        }catch(e){
            buttons(false);
            status.className='bad';
            status.textContent='Контроль готовности не запущен: '+e.message;
        }
    }
    async function applyVerifiedBackup(){
        const batchId=String(inspectId.value||'').trim();
        if(!/^[1-9]\d*$/.test(batchId)){
            status.className='bad';
            status.textContent='Сначала укажите ID копии и завершите контроль готовности до READY.';
            return;
        }
        if(!confirm('ВНИМАНИЕ: заменить рабочие файлы данными проверенной копии №'+batchId+'? Станок должен быть остановлен. При ошибке firmware немедленно запустит откат.'))return;
        const typed=prompt('Для подтверждения применения введите ID копии: '+batchId,'');
        if(typed!==batchId){
            status.className='warning';
            status.textContent='Применение отменено: введённый ID не совпал.';
            return;
        }
        clearTimeout(timer);
        buttons(true);
        status.className='warning';
        status.textContent='Запуск транзакционного применения копии №'+batchId+'. Не выключайте питание.';
        try{
            const body=new URLSearchParams({batch_id:batchId,confirmed:'APPLY'});
            const r=await fetch('/api/backup/remote/apply',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body}),j=await r.json();
            if(!r.ok)throw new Error(j.error||'apply_start_failed');
            pollApply(false);
        }catch(e){
            buttons(false);
            status.className='bad';
            status.textContent='Применение не запущено: '+e.message;
        }
    }
    async function discardStaging(){
        if(!confirm('Удалить только временно загруженные файлы восстановления? Рабочие данные не изменятся.'))return;
        clearTimeout(timer);
        buttons(true);
        try{
            const r=await fetch('/api/backup/remote/staging',{method:'DELETE'}),j=await r.json();
            if(!r.ok)throw new Error(j.error||'staging_cleanup_failed');
            buttons(false);
            status.className='ok';
            status.textContent='Временные файлы восстановления и страховочная копия удалены. Само удаление не изменяло рабочие файлы.';
        }catch(e){
            buttons(false);
            status.className='bad';
            status.textContent='Временные файлы не удалены: '+e.message;
        }
    }
    document.addEventListener('click',e=>{
        const b=e.target.closest('[data-remote-backup]');
        if(b)upload(b.dataset.remoteBackup);
    });
    full.onclick=batch;
    retention.onclick=applyRetention;
    inspect.onclick=inspectBatch;
    stage.onclick=stageBatch;
    plan.onclick=buildRestorePlan;
    rollback.onclick=buildRollbackSnapshot;
    preflight.onclick=runApplyPreflight;
    apply.onclick=applyVerifiedBackup;
    discard.onclick=discardStaging;
    new MutationObserver(enhance).observe(document.body,{childList:true,subtree:true});
    pollApply(true);
})();
