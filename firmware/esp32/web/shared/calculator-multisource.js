(()=>{
  const $=id=>document.getElementById(id), button=$('calculate');
  if(!button||document.getElementById('sourceComponents'))return;
  const oldDiameter=$('diameter'),oldStrands=$('strands');
  if(!oldDiameter||!oldStrands)return;
  const host=oldDiameter.closest('.fields')||oldDiameter.parentElement.parentElement;
  const block=document.createElement('div');
  block.id='sourceComponentsBlock';
  block.style.cssText='grid-column:1/-1';
  block.innerHTML='<label>Исходные жилы (до 5 разных диаметров)</label><div id="sourceComponents"></div><button id="addSourceComponent" type="button" style="margin-top:8px;background:#526879">+ Добавить другой диаметр</button><p class="muted">Каждая строка — диаметр и количество одинаковых параллельных жил. Например: 0,80 × 3 и 0,65 × 2.</p>';
  const firstContainer=oldDiameter.parentElement;
  const secondContainer=oldStrands.parentElement;
  host.insertBefore(block,firstContainer);
  firstContainer.remove();secondContainer.remove();
  const rows=$('sourceComponents');
  function add(d='0.80',n='1'){
    if(rows.children.length>=5)return;
    const row=document.createElement('div');
    row.className='cm-source-row';
    row.style.cssText='display:grid;grid-template-columns:1fr 1fr auto;gap:8px;align-items:end;margin-top:8px';
    row.innerHTML='<div><span class="muted">Диаметр, мм</span><input class="cm-source-diameter" inputmode="decimal" value="'+d+'"></div><div><span class="muted">Количество</span><input class="cm-source-strands" type="number" min="1" max="12" value="'+n+'"></div><button type="button" class="cm-source-remove" style="width:auto;margin:0;background:#b42318" aria-label="Удалить">×</button>';
    row.querySelector('.cm-source-remove').onclick=()=>{if(rows.children.length>1)row.remove()};
    rows.appendChild(row);
  }
  add(oldDiameter.value||'0.80',oldStrands.value||'3');
  $('addSourceComponent').onclick=()=>add('','1');
  const hundredths=v=>Math.round(Number(String(v).replace(',','.'))*100);
  const mm=v=>(Number(v)/100).toFixed(2).replace('.',',');
  button.onclick=async()=>{
    const sm=$('sourceMaterial').value,tm=$('targetMaterial').value;
    if(sm===tm)$('targetMaterial').value=sm==='AL'?'CU':'AL';
    const sourceRows=[...rows.querySelectorAll('.cm-source-row')];
    const q=new URLSearchParams({source_material:sm,target_material:$('targetMaterial').value,source_component_count:String(sourceRows.length)});
    for(let i=0;i<sourceRows.length;i++){
      const d=hundredths(sourceRows[i].querySelector('.cm-source-diameter').value);
      const n=Number(sourceRows[i].querySelector('.cm-source-strands').value);
      if(!Number.isInteger(d)||d<1||d>500||!Number.isInteger(n)||n<1||n>12){$('message').textContent='Проверьте диаметр и количество в строке '+(i+1)+'.';return}
      q.set('source_diameter_'+(i+1)+'_hundredths_mm',String(d));
      q.set('source_strands_'+(i+1),String(n));
    }
    $('message').textContent='Выполняется расчёт...';
    try{
      const r=await fetch('/api/calculator/conductor?'+q),data=await r.json();
      if(!r.ok)throw new Error(data.error==='calculator_not_configured'?'Сначала сохраните настройки калькулятора.':data.error||'Ошибка расчёта');
      $('message').textContent='Исходных групп: '+data.source_component_count+'. Каталог: '+data.catalogue_diameter_count+'. Найдено: '+data.recommendation_count+'.';
      const box=$('results');box.innerHTML='';
      if(!data.recommendations.length){box.innerHTML='<p class="muted">Нет подходящих вариантов в заданном допуске.</p>';return}
      data.recommendations.forEach(o=>{const stock=o.availability==='IN_STOCK',el=document.createElement('div');el.className=document.body.querySelector('nav')?'result':'result';const parts=(o.components||[]).map(c=>c.parallel_strands+' × '+mm(c.diameter_hundredths_mm)+' мм').join(' + ');el.innerHTML='<strong>№'+o.rank+' · '+parts+'</strong><p>Отклонение '+(o.deviation_permille/10).toFixed(1).replace('.',',')+' %</p><p class="'+(stock?'stock':'buy')+'">'+(stock?'Есть на складе':'Требуется закупить')+'</p>';box.appendChild(el)});
    }catch(e){$('message').textContent=e.message}
  };
})();
