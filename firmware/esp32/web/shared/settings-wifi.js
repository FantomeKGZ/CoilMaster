window.CMWifiPage=(()=>{
  const $=id=>document.getElementById(id);let items=[];
  const esc=v=>String(v??'').replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
  function show(text,ok){$('result').className='note '+(ok?'ok':'bad');$('result').textContent=text}
  function ensureStaticFields(){
    if($('useStaticIp'))return;
    const hidden=$('hidden').closest('label'),box=document.createElement('div');box.id='staticIpFields';box.hidden=true;
    box.innerHTML='<label>Внутренний IP ESP32</label><input id="localIp" inputmode="decimal" placeholder="192.168.1.50"><label>Маска подсети</label><input id="subnet" inputmode="decimal" value="255.255.255.0"><label>Шлюз (роутер)</label><input id="gateway" inputmode="decimal" placeholder="192.168.1.1"><label>DNS 1 (необязательно)</label><input id="dns1" inputmode="decimal" placeholder="192.168.1.1"><label>DNS 2 (необязательно)</label><input id="dns2" inputmode="decimal" placeholder="8.8.8.8"><p class="muted">Адрес должен быть свободен или зарезервирован в роутере. Точка CoilMaster останется на 192.168.4.1.</p>';
    const toggle=document.createElement('label');toggle.className='check';toggle.innerHTML='<input id="useStaticIp" type="checkbox"> Использовать статический IP';
    hidden.after(toggle,box);$('useStaticIp').onchange=()=>box.hidden=!$('useStaticIp').checked;
    document.querySelectorAll('p.muted').forEach(p=>{if(p.textContent.includes('Статическ')||p.textContent.includes('DHCP'))p.remove()});
  }
  function ensureScanFields(){
    if($('scanNetworks'))return;
    const ssid=$('ssid'),box=document.createElement('div');box.className='note';
    box.innerHTML='<button id="scanNetworks" type="button" class="secondary">Найти сети поблизости</button><div id="scanResult" class="muted">SSID можно ввести вручную или выбрать после поиска.</div>';
    ssid.parentNode.insertBefore(box,ssid.previousElementSibling);$('scanNetworks').onclick=startScan;
  }
  function signal(rssi){return rssi>=-55?'отличный':rssi>=-67?'хороший':rssi>=-75?'средний':'слабый'}
  function renderScan(items){
    const box=$('scanResult');
    if(!items.length){box.textContent='Доступные сети не найдены. Можно ввести SSID вручную.';return}
    box.innerHTML=items.map((x,i)=>'<div class="profile"><b>'+esc(x.ssid)+'</b><div class="muted">'+signal(Number(x.rssi))+' сигнал · '+x.rssi+' dBm · канал '+x.channel+' · '+(x.encrypted?'защищена':'без пароля')+'</div><button type="button" data-scan-index="'+i+'">Выбрать</button></div>').join('');
    box.querySelectorAll('[data-scan-index]').forEach(b=>b.onclick=()=>{const x=items[Number(b.dataset.scanIndex)];$('ssid').value=x.ssid;$('hidden').checked=false;$('password').focus();show('Сеть «'+x.ssid+'» выбрана. Введите пароль и сохраните профиль.',true)})
  }
  async function pollScan(){
    try{const r=await fetch('/api/network/scan',{cache:'no-store'}),j=await r.json();if(!r.ok)throw new Error(j.error||'scan_failed');if(j.active){$('scanResult').textContent='Поиск сетей…';setTimeout(pollScan,700);return}$('scanNetworks').disabled=false;renderScan(j.items||[])}catch(e){$('scanNetworks').disabled=false;$('scanResult').textContent='Ошибка поиска: '+e.message}
  }
  async function startScan(){
    $('scanNetworks').disabled=true;$('scanResult').textContent='Запуск поиска…';
    try{const r=await fetch('/api/network/scan',{method:'POST'}),j=await r.json();if(!r.ok)throw new Error(j.error||'scan_start_failed');pollScan()}catch(e){$('scanNetworks').disabled=false;$('scanResult').textContent=e.message==='network_connection_in_progress'?'Подождите завершения текущего подключения и повторите поиск.':'Ошибка запуска: '+e.message}
  }
  function clear(){['id','ssid','password','localIp','gateway','dns1','dns2'].forEach(x=>$(x).value='');$('subnet').value='255.255.255.0';$('priority').value='1';$('enabled').checked=true;$('hidden').checked=false;$('useStaticIp').checked=false;$('staticIpFields').hidden=true;$('formTitle').textContent='Добавить сеть'}
  function edit(id){const x=items.find(v=>v.id===id);if(!x)return;$('id').value=x.id;$('ssid').value=x.ssid;$('password').value='';$('password').placeholder=x.password_configured?'сохранён; пусто = не менять':'8–63 символа';$('priority').value=String(x.priority);$('enabled').checked=!!x.enabled;$('hidden').checked=!!x.hidden;$('useStaticIp').checked=!!x.use_static_ip;$('staticIpFields').hidden=!x.use_static_ip;$('localIp').value=x.local_ip||'';$('gateway').value=x.gateway||'';$('subnet').value=x.subnet||'255.255.255.0';$('dns1').value=x.dns1||'';$('dns2').value=x.dns2||'';$('formTitle').textContent='Изменить сеть №'+x.id;scrollTo({top:document.body.scrollHeight,behavior:'smooth'})}
  async function remove(id){if(!confirm('Удалить этот Wi‑Fi профиль?'))return;const body=new URLSearchParams({id:String(id)});try{const r=await fetch('/api/network/profiles/delete',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body}),j=await r.json();if(!r.ok)throw new Error(j.error||'delete_failed');clear();await load();show('Профиль удалён.',true)}catch(e){show('Ошибка удаления: '+e.message,false)}}
  function render(){window.cmWifiEdit=edit;window.cmWifiDelete=remove;$('profiles').innerHTML=items.length?items.map(x=>'<div class="profile"><b>'+esc(x.ssid)+'</b> · приоритет '+x.priority+'<div class="muted">'+(x.enabled?'включён':'выключен')+(x.hidden?' · скрытая сеть':'')+' · '+(x.use_static_ip?'статический IP '+esc(x.local_ip):'DHCP')+' · пароль '+(x.password_configured?'сохранён':'не задан')+'</div><div class="actions"><button onclick="cmWifiEdit('+x.id+')">Изменить</button><button class="danger" onclick="cmWifiDelete('+x.id+')">Удалить</button></div></div>').join(''):'Профили ещё не добавлены.'}
  async function load(){try{const [pr,sr]=await Promise.all([fetch('/api/network/profiles',{cache:'no-store'}),fetch('/api/system/network',{cache:'no-store'})]),p=await pr.json(),s=await sr.json();if(!pr.ok||!sr.ok)throw new Error('network_unavailable');items=p.items||[];render();$('runtime').innerHTML='<div class="row"><span>Состояние</span><b>'+esc(s.network_state)+'</b></div><div class="row"><span>AP</span><b>'+esc(s.ap_ssid)+' · '+esc(s.ap_ip)+'</b></div><div class="row"><span>STA</span><b>'+(s.sta_connected?esc(s.sta_ssid)+' · '+esc(s.sta_ip):(s.sta_connecting?'подключение…':'не подключена'))+'</b></div><div class="row"><span>Результат</span><b>'+esc(s.network_last_result)+'</b></div>'}catch(e){$('runtime').className='note bad';$('runtime').textContent='Сетевое состояние недоступно.';$('profiles').textContent='Профили недоступны.'}}
  async function save(e){e.preventDefault();const use=$('useStaticIp').checked,body=new URLSearchParams({id:$('id').value,ssid:$('ssid').value,password:$('password').value,priority:$('priority').value,enabled:$('enabled').checked?'1':'0',hidden:$('hidden').checked?'1':'0',use_static_ip:use?'1':'0',local_ip:use?$('localIp').value.trim():'',gateway:use?$('gateway').value.trim():'',subnet:use?$('subnet').value.trim():'',dns1:use?$('dns1').value.trim():'',dns2:use?$('dns2').value.trim():''});try{const r=await fetch('/api/network/profiles',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body}),j=await r.json();if(!r.ok)throw new Error(j.error||'save_failed');clear();await load();show('Профиль сохранён. Подключение запущено.',true)}catch(e){show('Ошибка: '+e.message,false)}}
  async function reconnect(){try{const r=await fetch('/api/network/reconnect',{method:'POST'}),j=await r.json();if(!r.ok)throw new Error(j.error||'reconnect_failed');setTimeout(load,800)}catch(e){show('Ошибка подключения: '+e.message,false)}}
  function start(variant){localStorage.setItem('cm-ui-version',variant);ensureScanFields();ensureStaticFields();$('form').onsubmit=save;$('cancel').onclick=clear;$('reconnect').onclick=reconnect;load()}
  return{start};
})();
