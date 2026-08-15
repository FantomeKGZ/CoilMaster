(()=>{
  const box=document.getElementById('pricingHistory');
  if(!box)return;
  const pager=document.createElement('div');
  pager.style.cssText='display:flex;align-items:center;justify-content:center;gap:10px;margin-top:12px';
  pager.innerHTML='<button id="pricingHistoryPrev" type="button" style="width:auto">← Назад</button><span id="pricingHistoryPage" class="muted">Страница 1</span><button id="pricingHistoryNext" type="button" style="width:auto">Далее →</button>';
  box.after(pager);
  const prev=document.getElementById('pricingHistoryPrev'),next=document.getElementById('pricingHistoryNext'),label=document.getElementById('pricingHistoryPage');
  let repair='',cursor=0,nextCursor=0,hasMore=false,page=1,back=[];
  const safe=v=>String(v??'').replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
  const cash=v=>(Number(v)/100).toLocaleString('ru-RU',{minimumFractionDigits:2,maximumFractionDigits:2})+' сом';
  const when=v=>{if(!v)return'—';const d=new Date(v);return Number.isNaN(d.getTime())?String(v):d.toLocaleString('ru-RU')};
  async function paged(reset=false){
    const id=document.getElementById('repairId').value;
    if(!id){box.className='muted';box.textContent='Ремонт не выбран.';pager.hidden=true;return}
    if(reset||id!==repair){repair=id;cursor=0;nextCursor=0;hasMore=false;page=1;back=[]}
    pager.hidden=false;box.className='muted';box.textContent='Загрузка страницы истории…';
    try{
      const q=new URLSearchParams({repair_id:id,cursor:String(cursor),limit:'20'}),r=await fetch('/api/repairs/pricing-history?'+q,{cache:'no-store'}),j=await r.json();
      if(!r.ok)throw new Error(j.error||'Ошибка');
      const items=Array.isArray(j.items)?j.items:[],trusted=j.source==='APPEND_ONLY_LOG'&&j.current_pricing_source==='LATEST_APPEND_ONLY_REVISION'&&j.history_count_matches_current===true&&j.history_latest_values_match_current===true&&j.history_latest_timestamp_matches_current===true&&j.history_matches_current_pricing===true&&Number(j.count)===items.length&&Number(j.latest_revision)===Number(j.total_count);
      hasMore=j.has_more===true;nextCursor=hasMore?Number(j.next_cursor):0;
      if(hasMore&&(!Number.isSafeInteger(nextCursor)||nextCursor<=cursor))throw new Error('invalid_pricing_history_cursor');
      box.className=trusted?'':'warning';
      box.innerHTML=items.length?items.slice().reverse().map((x,i)=>'<div class="revision"><div class="revision-head"><b>Редакция №'+safe(x.revision)+(!hasMore&&i===0?'<span class="current">'+(trusted?'текущая':'последняя в журнале')+'</span>':'')+'</b><span>'+safe(when(x.timestamp))+'</span></div><div>Работа: <b>'+cash(x.labour_cost_minor)+'</b></div><div>Цена клиенту: <b>'+cash(x.client_price_minor)+'</b></div><div class="muted">'+safe(x.currency||'KGS')+' · журнал ESP32</div></div>').join(''):'На этой странице редакций нет.';
      if(!trusted&&items.length)box.innerHTML='<div class="warning">История не подтверждает активную цену калькуляции.</div>'+box.innerHTML;
      prev.disabled=back.length===0;next.disabled=!hasMore;label.textContent='Страница '+page+' · '+items.length+' из '+Number(j.total_count||0);
    }catch(e){box.className='bad';box.textContent='Не удалось загрузить историю цены: '+e.message;prev.disabled=true;next.disabled=true}
  }
  window.loadHistory=()=>paged(repair!==document.getElementById('repairId').value);
  prev.onclick=()=>{if(!back.length)return;cursor=back.pop();page=Math.max(1,page-1);paged(false)};
  next.onclick=()=>{if(!hasMore)return;back.push(cursor);cursor=nextCursor;page+=1;paged(false)};
  paged(true);
})();
