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
    manifestState.parentNode.insertBefore(status,manifestState.nextSibling);
    status.insertAdjacentElement('afterend',full);
    full.insertAdjacentElement('afterend',retention);

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
    document.addEventListener('click',e=>{
        const b=e.target.closest('[data-remote-backup]');
        if(b)upload(b.dataset.remoteBackup);
    });
    full.onclick=batch;
    retention.onclick=applyRetention;
    new MutationObserver(enhance).observe(document.body,{childList:true,subtree:true});
    pollBatch();
})();
