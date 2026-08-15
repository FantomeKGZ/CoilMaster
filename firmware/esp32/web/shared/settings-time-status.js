(()=>{
    const network=document.getElementById('networkSummary');
    if(!network||document.getElementById('rtcSummary'))return;
    const panel=document.createElement('div');
    panel.id='rtcSummary';
    panel.className='note';
    panel.textContent='Загрузка состояния часов DS3231…';
    network.insertAdjacentElement('afterend',panel);

    const row=(label,value)=>{
        const line=document.createElement('div');
        line.className='row';
        const name=document.createElement('span');
        name.textContent=label;
        const result=document.createElement('b');
        result.textContent=value;
        line.append(name,result);
        return line;
    };
    async function load(){
        panel.className='note';
        panel.textContent='Обновление состояния часов…';
        try{
            const response=await fetch('/api/system/time',{cache:'no-store'});
            const data=await response.json();
            if(!response.ok)throw new Error(data.error||'rtc_status_unavailable');
            panel.textContent='';
            const title=document.createElement('b');
            title.textContent='🕒 Часы DS3231';
            panel.appendChild(title);
            panel.appendChild(row('Модуль',data.detected?'обнаружен':'не найден'));
            panel.appendChild(row('Дата и время',data.time_valid&&data.local_time
                ?data.local_time.replace('T',' '):'требуется установка'));
            panel.appendChild(row('Расписание backup',data.scheduling_ready
                ?'готово':'ещё не включено'));
            const refresh=document.createElement('button');
            refresh.type='button';
            refresh.textContent='Обновить время';
            refresh.onclick=load;
            panel.appendChild(refresh);
            if(!data.detected||!data.time_valid)panel.className='note warn';
        }catch(error){
            panel.className='note bad';
            panel.textContent='Не удалось прочитать состояние DS3231: '+error.message;
        }
    }
    load();
})();
