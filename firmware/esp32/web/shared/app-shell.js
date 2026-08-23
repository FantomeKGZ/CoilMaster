(()=>{
'use strict';
if(window.CMApp&&window.CMApp.version)return;

const WEB_VERSION='2026.08.22-phase9';
const uiMode=location.pathname.startsWith('/mobile/')?'mobile':'desktop';
const esc=value=>String(value??'').replace(/[&<>"']/g,ch=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[ch]));
const sleep=ms=>new Promise(resolve=>setTimeout(resolve,ms));

const sections=[
 ['🏠','Главная',''],['🧰','Ремонты','repairs.html'],['👤','Клиенты','clients.html'],
 ['📊','Двигатели','motors.html'],['🔌','Arduino','arduino-windings.html'],
 ['🧮','Калькулятор','calculator.html'],['📦','Склад','warehouse.html'],
 ['💰','Калькуляция','costing.html'],['📈','Отчёты','reports.html'],
 ['💾','Backup','backup.html'],['⚙️','Настройки','settings.html'],
 ['📚','Справочник',`/sites/reference/${uiMode}/`]
];

function sectionHref(file){
 if(file&&file.startsWith('/'))return file;
 return file?`/${uiMode}/${file}`:`/${uiMode}/`;
}
function sectionActive(file){
 const pathname=location.pathname;
 if(!file)return pathname===`/${uiMode}/`||pathname===`/${uiMode}/index.html`;
 if(file==='motors.html')return pathname.includes('/motor-')||pathname.endsWith('/motors.html');
 if(file==='settings.html')return pathname.includes('/settings');
 if(file==='arduino-windings.html')return pathname.includes('arduino-windings');
 return pathname.endsWith('/'+file);
}

function ensureStyle(){
 if(document.getElementById('cm-app-shell-style'))return;
 const style=document.createElement('style');
 style.id='cm-app-shell-style';
 style.textContent=`
 .cm-shell{background:#fff;border:1px solid #dbe4eb;border-radius:12px;padding:10px 12px;margin:0 0 14px;box-shadow:0 1px 5px #17212b12;font-family:Arial,sans-serif;color:#17212b}
 .cm-shell-nav{display:flex;gap:5px;overflow-x:auto;padding:0 0 9px;margin:0 0 9px;border-bottom:1px solid #e5ebf0;scrollbar-width:thin}.cm-shell-nav a{display:inline-flex;gap:5px;align-items:center;white-space:nowrap;padding:7px 9px;border-radius:8px;text-decoration:none;color:#31506a;font-size:13px;font-weight:700}.cm-shell-nav a:hover,.cm-shell-nav a:focus{background:#edf4fa;outline:none}.cm-shell-nav a.active{background:#1769aa;color:#fff}
 .cm-shell-row{display:flex;align-items:center;gap:10px;flex-wrap:wrap}.cm-shell-breadcrumbs{font-size:13px;color:#687580;flex:1 1 260px}.cm-shell-breadcrumbs a{color:#1769aa;text-decoration:none}.cm-shell-tools{display:flex;align-items:center;gap:8px;flex:1 1 420px;justify-content:flex-end;flex-wrap:wrap}.cm-shell-clock,.cm-shell-version{font-size:12px;color:#53606b;white-space:nowrap}.cm-shell-clock[data-state="bad"]{color:#b42318}.cm-shell-search-wrap{position:relative;min-width:230px;max-width:420px;flex:1}.cm-shell-search{width:100%!important;margin:0!important;padding:9px 11px!important;font-size:14px!important;border:1px solid #cbd5df!important;border-radius:8px!important;background:#fff!important;color:#17212b!important}.cm-shell-results{display:none;position:absolute;z-index:1000;left:0;right:0;top:calc(100% + 5px);max-height:360px;overflow:auto;background:#fff;border:1px solid #cbd5df;border-radius:10px;box-shadow:0 8px 28px #17212b2b;padding:7px}.cm-shell-results.open{display:block}.cm-shell-result{display:block;padding:9px;border-radius:7px;text-decoration:none;color:#17212b}.cm-shell-result:hover,.cm-shell-result:focus{background:#edf4fa;outline:none}.cm-shell-result small{display:block;color:#687580;margin-top:2px}.cm-shell-empty{padding:10px;color:#687580;font-size:13px}.cm-toast-region{position:fixed;right:16px;bottom:16px;z-index:2000;display:grid;gap:8px;max-width:min(420px,calc(100vw - 32px))}.cm-toast{background:#17212b;color:#fff;padding:11px 13px;border-radius:10px;box-shadow:0 5px 20px #0003;font:14px Arial,sans-serif}.cm-toast.bad{background:#8b2d2d}.cm-toast.ok{background:#17683a}.cm-recent{margin-top:7px;padding-top:7px;border-top:1px solid #e5ebf0}.cm-recent-title{font-size:12px;font-weight:bold;color:#687580;padding:3px 9px}
 @media(max-width:700px){.cm-shell{border-radius:0;margin:0 0 10px}.cm-shell-tools{justify-content:flex-start}.cm-shell-search-wrap{order:3;flex-basis:100%;max-width:none}.cm-shell-version{font-size:11px}.cm-shell-nav a{font-size:12px;padding:7px 8px}}
 `;
 document.head.appendChild(style);
}

function pageLabel(pathname){
 const map={
  '/desktop/':'Главная','/mobile/':'Главная',
  '/desktop/repairs.html':'Ремонты','/mobile/repairs.html':'Ремонты',
  '/desktop/clients.html':'Клиенты','/mobile/clients.html':'Клиенты',
  '/desktop/motors.html':'Двигатели','/mobile/motors.html':'Двигатели',
  '/desktop/motor-details.html':'Двигатель','/mobile/motor-details.html':'Двигатель',
  '/desktop/arduino-windings.html':'Arduino архив','/mobile/arduino-windings.html':'Arduino архив',
  '/desktop/calculator.html':'Калькулятор','/mobile/calculator.html':'Калькулятор',
  '/desktop/warehouse.html':'Склад','/mobile/warehouse.html':'Склад',
  '/desktop/costing.html':'Калькуляция','/mobile/costing.html':'Калькуляция',
  '/desktop/reports.html':'Отчёты','/mobile/reports.html':'Отчёты',
  '/desktop/backup.html':'Резервная копия','/mobile/backup.html':'Резервная копия',
  '/desktop/settings.html':'Настройки','/mobile/settings.html':'Настройки',
  '/desktop/settings-ftp.html':'FTP/Web recovery','/mobile/settings-ftp.html':'FTP/Web recovery',
  '/desktop/settings-hall.html':'Датчик Холла','/mobile/settings-hall.html':'Датчик Холла',
  '/desktop/settings-time.html':'Время','/mobile/settings-time.html':'Время',
  '/desktop/settings-wifi.html':'Wi‑Fi','/mobile/settings-wifi.html':'Wi‑Fi',
  '/desktop/winding-history.html':'История намотки','/mobile/winding-history.html':'История намотки',
  '/desktop/writeoff.html':'Списание провода','/mobile/writeoff.html':'Списание провода'
 };
 return map[pathname]||document.title||'CoilMaster';
}

function breadcrumbs(){
 const home=uiMode==='mobile'?'/mobile/':'/desktop/';
 const label=pageLabel(location.pathname);
 const bits=[`<a href="${home}">CoilMaster</a>`];
 if(location.pathname!==home)bits.push(esc(label));
 const params=new URLSearchParams(location.search);
 for(const key of ['repair_id','motor_id','client_id','session_id','run_id']){
  const value=params.get(key);
  if(value)bits.push(`${esc(key.replace('_id',''))} #${esc(value)}`);
 }
 return bits.join(' / ');
}

function navHtml(){return sections.map(([icon,label,file])=>`<a href="${sectionHref(file)}"${sectionActive(file)?' class="active" aria-current="page"':''}>${icon} ${esc(label)}</a>`).join('')}

const recentKey='cm-recent-items-v1';
function loadRecent(){
 try{const parsed=JSON.parse(localStorage.getItem(recentKey)||'[]');return Array.isArray(parsed)?parsed:[]}catch(_){return[]}
}
function saveRecent(item){
 if(!item||!item.href)return;
 const items=loadRecent().filter(x=>x&&x.href!==item.href);
 items.unshift(item);
 localStorage.setItem(recentKey,JSON.stringify(items.slice(0,8)));
}
function rememberCurrent(){
 const params=new URLSearchParams(location.search);
 const identity=['repair_id','motor_id','client_id','session_id','run_id'].find(k=>params.get(k));
 if(!identity)return;
 saveRecent({href:location.pathname+location.search,label:document.title||pageLabel(location.pathname),meta:`${identity.replace('_id','')} #${params.get(identity)}`});
}

function toast(message,type=''){
 let region=document.querySelector('.cm-toast-region');
 if(!region){region=document.createElement('div');region.className='cm-toast-region';region.setAttribute('aria-live','polite');document.body.appendChild(region)}
 const item=document.createElement('div');
 item.className='cm-toast'+(type?' '+type:'');
 item.textContent=String(message||'');
 region.appendChild(item);
 setTimeout(()=>item.remove(),4200);
}

function errorText(value){
 const code=String(value||'unknown');
 const map={
  network_error:'Нет связи с ESP32.',
  storage_unavailable:'Хранилище microSD недоступно.',
  invalid_paging_cursor:'Получен некорректный курсор страницы.',
  manual_review_required:'Требуется ручная проверка состояния задания.',
  machine_busy:'Операция недоступна во время активной намотки.'
 };
 return map[code]||code.replaceAll('_',' ');
}

function resultLink(item){
 const a=document.createElement('a');
 a.className='cm-shell-result';
 a.href=item.href;
 a.innerHTML=`${esc(item.label)}<small>${esc(item.meta||'')}</small>`;
 return a;
}

async function fetchJson(url){
 const response=await fetch(url,{cache:'no-store'});
 const data=await response.json();
 if(!response.ok)throw new Error(data.error||'request_failed');
 return data;
}

async function searchMotors(query){
 const data=await fetchJson('/api/motors?'+new URLSearchParams({limit:'6',q:query}));
 return (data.items||[]).map(x=>({
  label:[x.manufacturer,x.model].filter(Boolean).join(' ').trim()||x.name||`Двигатель #${x.motor_id}`,
  meta:`Двигатель #${x.motor_id}${x.coil_program?' · '+x.coil_program:''}`,
  href:`/${uiMode}/motor-details.html?motor_id=${encodeURIComponent(x.motor_id)}`
 }));
}

async function searchClients(query){
 const data=await fetchJson('/api/search/clients?'+new URLSearchParams({limit:'6',q:query}));
 return (data.items||[]).map(x=>({
  label:x.name||`Клиент #${x.client_id}`,
  meta:`Клиент #${x.client_id}${x.phone?' · '+x.phone:''}`,
  href:`/${uiMode}/repairs.html?client_id=${encodeURIComponent(x.client_id)}`
 }));
}

async function searchRepairs(query){
 const data=await fetchJson('/api/search/repairs?'+new URLSearchParams({limit:'6',q:query}));
 return (data.items||[]).map(x=>({
  label:`Ремонт #${x.repair_id}`,
  meta:`${x.current_status||x.status||'OPEN'} · клиент #${x.client_id} · двигатель #${x.motor_id}${x.complaint?' · '+x.complaint:''}`,
  href:`/${uiMode}/winding-history.html?repair_id=${encodeURIComponent(x.repair_id)}`
 }));
}

async function globalSearch(query){
 const q=String(query||'').trim();
 if(q.length<2)return [];
 const settled=await Promise.allSettled([searchMotors(q),searchClients(q),searchRepairs(q)]);
 return settled.flatMap(x=>x.status==='fulfilled'?x.value:[]).slice(0,14);
}

function renderRecent(results){
 const items=loadRecent();
 if(!items.length)return;
 const block=document.createElement('div');block.className='cm-recent';
 const title=document.createElement('div');title.className='cm-recent-title';title.textContent='Недавние';block.appendChild(title);
 for(const item of items.slice(0,5))block.appendChild(resultLink(item));
 results.appendChild(block);
}

let clockBaseMs=0,clockBasePerf=0,clockTimer=0,resyncTimer=0;
function parseLocalTime(localTime){
 if(!localTime)return NaN;
 const match=String(localTime).match(/^(\d{4})-(\d{2})-(\d{2})T(\d{2}):(\d{2}):(\d{2})/);
 if(!match)return NaN;
 return Date.UTC(Number(match[1]),Number(match[2])-1,Number(match[3]),Number(match[4]),Number(match[5]),Number(match[6]));
}
function formatClock(ms){
 const d=new Date(ms);
 return `${String(d.getUTCDate()).padStart(2,'0')}.${String(d.getUTCMonth()+1).padStart(2,'0')}.${d.getUTCFullYear()} ${String(d.getUTCHours()).padStart(2,'0')}:${String(d.getUTCMinutes()).padStart(2,'0')}:${String(d.getUTCSeconds()).padStart(2,'0')}`;
}
async function syncClock(node){
 try{
  const data=await fetchJson('/api/system/time');
  const parsed=parseLocalTime(data.local_time);
  if(!data.time_valid||!Number.isFinite(parsed))throw new Error('time_unavailable');
  clockBaseMs=parsed;clockBasePerf=performance.now();node.dataset.state='ok';node.title=data.detected?'DS3231 / device time':'Device time';
 }catch(_){node.dataset.state='bad';node.textContent='Время недоступно';return}
 const tick=()=>{node.textContent=formatClock(clockBaseMs+(performance.now()-clockBasePerf))};
 tick();clearInterval(clockTimer);clockTimer=setInterval(tick,1000);
}

async function loadVersion(node){
 let firmware='FW unknown',web=WEB_VERSION;
 try{
  const build=await fetchJson('/api/system/build');
  const sha=String(build.firmware_git_sha||'unknown');
  const branch=String(build.firmware_git_branch||'');
  firmware=`FW ${sha}${branch&&branch!=='unknown'?' · '+branch:''}`;
  web=String(build.web_contract_version||WEB_VERSION);
  node.title=build.firmware_build_utc&&build.firmware_build_utc!=='unknown'?`Firmware built ${build.firmware_build_utc}`:'Firmware build time unavailable';
 }catch(_){}
 node.textContent=`${firmware} · Web ${web}`;
}

function applyIncomingSearch(){
 const q=new URLSearchParams(location.search).get('cm_search');
 if(!q)return;
 const field=document.getElementById('search');
 if(!field)return;
 field.value=q;
 field.dispatchEvent(new Event('input',{bubbles:true}));
 const find=document.getElementById('find');
 if(find)setTimeout(()=>find.click(),0);
}

function shellHost(){return document.querySelector('main')||document.querySelector('.content')||document.body}
function buildShell(){
 ensureStyle();
 if(document.getElementById('cmAppShell'))return;
 const shell=document.createElement('section');shell.id='cmAppShell';shell.className='cm-shell';shell.setAttribute('aria-label','CoilMaster navigation utilities');
 shell.innerHTML=`<nav class="cm-shell-nav" aria-label="Основные разделы">${navHtml()}</nav><div class="cm-shell-row"><div class="cm-shell-breadcrumbs">${breadcrumbs()}</div><div class="cm-shell-tools"><div class="cm-shell-search-wrap"><input class="cm-shell-search" id="cmGlobalSearch" type="search" autocomplete="off" placeholder="Поиск: двигатель, клиент, ремонт" aria-label="Глобальный поиск"><div id="cmGlobalResults" class="cm-shell-results"></div></div><span id="cmDeviceClock" class="cm-shell-clock">Время…</span><span id="cmBuildVersion" class="cm-shell-version">Версия…</span></div></div>`;
 const host=shellHost();host.insertBefore(shell,host.firstChild);
 const input=shell.querySelector('#cmGlobalSearch'),results=shell.querySelector('#cmGlobalResults');
 let token=0;
 const close=()=>results.classList.remove('open');
 input.addEventListener('focus',()=>{if(!input.value.trim()){results.textContent='';renderRecent(results);if(results.childNodes.length)results.classList.add('open')}});
 input.addEventListener('input',async()=>{
  const current=++token,q=input.value.trim();results.textContent='';
  if(q.length<2){renderRecent(results);results.classList.toggle('open',results.childNodes.length>0);return}
  results.classList.add('open');results.innerHTML='<div class="cm-shell-empty">Поиск…</div>';
  await sleep(180);if(current!==token)return;
  const found=await globalSearch(q);if(current!==token)return;
  results.textContent='';for(const item of found)results.appendChild(resultLink(item));
  if(!found.length)results.innerHTML='<div class="cm-shell-empty">Совпадений не найдено.</div>';
 });
 document.addEventListener('click',event=>{if(!shell.contains(event.target))close()});
 document.addEventListener('keydown',event=>{if(event.key==='Escape'){close();input.blur()}if((event.ctrlKey||event.metaKey)&&event.key.toLowerCase()==='k'){event.preventDefault();input.focus();input.select()}});
 syncClock(shell.querySelector('#cmDeviceClock'));
 loadVersion(shell.querySelector('#cmBuildVersion'));
 clearInterval(resyncTimer);resyncTimer=setInterval(()=>syncClock(shell.querySelector('#cmDeviceClock')),45000);
 document.addEventListener('visibilitychange',()=>{if(!document.hidden)syncClock(shell.querySelector('#cmDeviceClock'))});
 applyIncomingSearch();
}

window.CMApp={version:WEB_VERSION,toast,errorText,search:globalSearch,rememberRecent:saveRecent};
rememberCurrent();
if(document.readyState==='loading')document.addEventListener('DOMContentLoaded',buildShell,{once:true});else buildShell();
})();