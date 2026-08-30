window.CMRemoteBackupPage=(()=>{
    const $=id=>document.getElementById(id);
    const pad=value=>String(Number(value)||0).padStart(2,'0');
    function show(text,ok){
        $('result').className='note '+(ok?'ok':'bad');
        $('result').textContent=text;
    }
    function ftpShow(text,ok){
        $('ftpResult').className='note '+(ok?'ok':'bad');
        $('ftpResult').textContent=text;
    }
    function ftpResultText(value){
        const code=String(value||'not_started');
        const labels={
            not_started:'ещё не запускался',
            activity_state_not_safe:'безопасный простой не подтверждён',
            web_root_create_failed:'не удалось создать каталог /web',
            web_root_unavailable:'каталог /web недоступен',
            automatic_recovery_started:'аварийное восстановление запущено',
            operator_started:'запущен оператором',
            stopped:'остановлен',
            automatic_recovery_complete:'восстановление сайта завершено',
            stopped_activity_not_safe:'остановлен: станок больше не в безопасном простое',
            client_disconnected:'FTP-клиент отключён',
            client_timeout:'тайм-аут FTP-клиента',
            non_local_client_rejected:'отклонено подключение не из локальной сети',
            client_connected:'FTP-клиент подключён',
            authenticated:'вход выполнен',
            authentication_failed:'ошибка логина или пароля',
            data_connection_timeout:'тайм-аут канала передачи данных',
            data_client_disconnected:'канал передачи данных отключён',
            storage_write_failed:'ошибка записи на microSD',
            target_backup_failed:'не удалось подготовить резервную замену файла',
            atomic_rename_failed:'не удалось атомарно заменить файл',
            upload_completed:'загрузка файла завершена',
            listing_completed:'чтение списка файлов завершено',
            transfer_aborted:'передача прервана',
            operator_stopped:'остановлен оператором'
        };
        return labels[code]||('Неизвестно ('+code+')');
    }
    function ensureScheduleControls(){
        if($('scheduleEnabled'))return;
        const enabledLabel=$('enabled').closest('label');
        const scheduleLabel=document.createElement('label');
        scheduleLabel.className='check';
        const checkbox=document.createElement('input');
        checkbox.id='scheduleEnabled';
        checkbox.type='checkbox';
        scheduleLabel.append(checkbox,document.createTextNode(' Ежедневная автоматическая копия'));
        const timeLabel=document.createElement('label');
        timeLabel.textContent='Время автоматической копии';
        const time=document.createElement('input');
        time.id='scheduleTime';
        time.type='time';
        time.value='02:00';
        time.required=true;
        enabledLabel.parentNode.insertBefore(scheduleLabel,enabledLabel);
        enabledLabel.parentNode.insertBefore(timeLabel,enabledLabel);
        enabledLabel.parentNode.insertBefore(time,enabledLabel);
        const status=document.createElement('div');
        status.id='scheduleStatus';
        status.className='note';
        status.textContent='Расписание выключено.';
        $('form').insertAdjacentElement('afterend',status);
    }
    function scheduleText(configuration){
        const labels={
            DISABLED:'Расписание выключено.',
            WAITING_TIME:'Ожидание заданного времени.',
            WAITING_STA:'Ожидание подключения к Wi‑Fi роутера.',
            WAITING_IDLE:'Ожидание безопасного простоя станка.',
            RTC_INVALID:'Часы DS3231 недействительны; автоматическая копия заблокирована.',
            RUNNING:'Выполняется плановая резервная копия.',
            COMPLETED_TODAY:'Плановая копия за сегодня выполнена.',
            SETTINGS_UNAVAILABLE:'Настройки расписания недоступны.',
            STATE_WRITE_FAILED:'Копия создана, но дата выполнения не сохранена.',
            FAILED:'Плановая копия завершилась ошибкой.'
        };
        let text=labels[configuration.schedule_state]||
            ('Неизвестное состояние расписания ('+String(configuration.schedule_state||'unknown')+')');
        if(configuration.last_scheduled_date){
            const value=String(configuration.last_scheduled_date);
            if(value.length===8)text+=' Последняя: '+value.slice(6,8)+'.'+value.slice(4,6)+'.'+value.slice(0,4)+'.';
        }
        $('scheduleStatus').className='note '+
            (/FAILED|INVALID|UNAVAILABLE/.test(String(configuration.schedule_state||''))?'bad':'');
        $('scheduleStatus').textContent=text;
    }
    async function load(){
        try{
            const [cr,nr,fr]=await Promise.all([
                fetch('/api/backup/remote/configuration',{cache:'no-store'}),
                fetch('/api/system/network',{cache:'no-store'}),
                fetch('/api/ftp/status',{cache:'no-store'})
            ]);
            const c=await cr.json(),n=await nr.json(),f=await fr.json();
            if(!cr.ok)throw new Error(c.error||'settings_unavailable');
            $('host').value=c.host||'';
            $('port').value=c.port||21;
            $('username').value=c.username||'';
            $('directory').value=c.remote_directory||'/CoilMaster';
            $('retention').value=c.retention_count||7;
            $('enabled').checked=!!c.enabled;
            $('scheduleEnabled').checked=!!c.schedule_enabled;
            $('scheduleTime').value=pad(c.schedule_hour)+':'+pad(c.schedule_minute);
            $('password').placeholder=c.password_configured
                ?'сохранён; пусто = не менять':'введите пароль';
            show(c.configured?'Настройки загружены. Пароль скрыт.':'Укажите FTP-сервер роутера.',true);
            scheduleText(c);
            if(fr.ok){
                const addresses=[f.ap_address||f.address];if(f.sta_available&&f.sta_address)addresses.push(f.sta_address);$('device').innerHTML='<div class="row"><span>Сервер /web</span><b>'+(f.enabled?'включён':'выключен')+'</b></div><div class="row"><span>Адреса</span><b>'+addresses.map(a=>a+':'+f.port).join(' · ')+'</b></div><div class="row"><span>/web готов</span><b>'+(f.web_root_usable?'да':'нет')+'</b></div><div class="row"><span>Клиент</span><b>'+(f.client_connected?'подключён':'нет')+'</b></div><div class="row"><span>Режим</span><b>'+(f.automatic_recovery?'аварийный автозапуск':'ручной')+'</b></div><div class="row"><span>Последний результат</span><b>'+ftpResultText(f.last_result)+'</b></div>';
            }else if(nr.ok)$('device').textContent='Статус FTP-сервера недоступен.';
        }catch(error){
            show('Ошибка загрузки: '+error.message,false);
        }
    }
    async function save(event){
        event.preventDefault();
        const time=String($('scheduleTime').value||'02:00').split(':');
        const body=new URLSearchParams({
            enabled:$('enabled').checked?'1':'0',
            host:$('host').value.trim(),
            port:$('port').value,
            username:$('username').value.trim(),
            password:$('password').value,
            remote_directory:$('directory').value.trim(),
            retention_count:$('retention').value,
            schedule_enabled:$('scheduleEnabled').checked?'1':'0',
            schedule_hour:String(Number(time[0])),
            schedule_minute:String(Number(time[1]))
        });
        try{
            const response=await fetch('/api/backup/remote/configuration',{
                method:'POST',
                headers:{'Content-Type':'application/x-www-form-urlencoded'},
                body
            });
            const result=await response.json();
            if(!response.ok)throw new Error(result.error||'save_failed');
            $('password').value='';
            show('Настройки и расписание сохранены. Пароль скрыт.',true);
            await load();
        }catch(error){
            show('Ошибка сохранения: '+error.message,false);
        }
    }
    async function test(){
        show('Проверка FTP…',true);
        try{
            const response=await fetch('/api/backup/remote/test',{method:'POST'});
            const result=await response.json();
            if(!response.ok)throw new Error(result.error||'test_failed');
            show('Вход выполнен, каталог доступен.',true);
        }catch(error){
            const labels={sta_not_connected:'ESP32 ещё не подключена к Wi‑Fi роутера.',active_winding:'Проверка запрещена во время активной намотки.',activity_state_unavailable:'Безопасное состояние станка не подтверждено.',ftp_connect_failed:'FTP-сервер не отвечает.',ftp_authentication_failed:'FTP отклонил логин или пароль.',ftp_remote_directory_unavailable:'Указанный каталог недоступен.'};
            show(labels[error.message]||('Ошибка проверки: '+error.message),false);
        }
    }
    async function ftpAction(action){
        ftpShow(action==='start'?'Запуск FTP…':'Остановка FTP…',true);
        try{
            const response=await fetch('/api/ftp/'+action,{method:'POST'});
            const result=await response.json();
            if(!response.ok)throw new Error(result.error||'ftp_action_failed');
            ftpShow(action==='start'?'FTP-сервер запущен.':'FTP-сервер остановлен.',true);
            await load();
        }catch(error){
            ftpShow(error.message==='ftp_start_blocked'
                ?'Запуск запрещён: безопасный простой станка не подтверждён.'
                :'Ошибка: '+error.message,false);
        }
    }
    function start(variant){
        localStorage.setItem('cm-ui-version',variant);
        ensureScheduleControls();
        $('form').onsubmit=save;
        $('test').onclick=test;
        $('ftpStart').onclick=()=>ftpAction('start');
        $('ftpStop').onclick=()=>ftpAction('stop');
        load();
    }
    return{start};
})();
