(function(){
  const MODE_KEY='cm-ui-version';
  const CATALOG_URL='/sites/reference/shared/catalog.json';
  const MAX_RESULTS=60;

  function setMode(mode){
    if(mode==='desktop'||mode==='mobile') localStorage.setItem(MODE_KEY,mode);
  }

  function wrapTables(root){
    root.querySelectorAll('table').forEach(function(table){
      if(table.parentElement&&table.parentElement.classList.contains('cm-reference-table-scroll')) return;
      const wrap=document.createElement('div');
      wrap.className='cm-reference-table-scroll';
      table.parentNode.insertBefore(wrap,table);
      wrap.appendChild(table);
    });
  }

  function normalizeModeLinks(root){
    root.querySelectorAll('[data-reference-mode]').forEach(function(link){
      link.addEventListener('click',function(){setMode(link.getAttribute('data-reference-mode'));});
    });
  }

  function normalizeSearch(value){
    return String(value||'').toLocaleLowerCase('ru-RU').replace(/ё/g,'е').trim();
  }

  function pageHref(mode,path){
    return encodeURI('/sites/reference/'+mode+'/pages/'+path);
  }

  function renderResults(catalog,mode,query,container,status){
    const needle=normalizeSearch(query);
    container.textContent='';
    if(!needle){
      status.textContent='В каталоге '+catalog.length+' страниц. Введите серию, марку двигателя или часть названия.';
      return;
    }

    const matches=catalog.filter(function(entry){
      if(entry[mode]!==true) return false;
      return normalizeSearch(entry.title+' '+entry.path).includes(needle);
    });

    status.textContent=matches.length
      ? 'Найдено: '+matches.length+(matches.length>MAX_RESULTS?' · показаны первые '+MAX_RESULTS:'')
      : 'Ничего не найдено. Попробуйте часть марки или названия.';

    matches.slice(0,MAX_RESULTS).forEach(function(entry){
      const link=document.createElement('a');
      link.className='cm-reference-search-result';
      link.href=pageHref(mode,entry.path);
      const title=document.createElement('b');
      title.textContent=entry.title;
      const path=document.createElement('small');
      path.textContent=entry.path;
      link.appendChild(title);
      link.appendChild(path);
      container.appendChild(link);
    });
  }

  function initCatalogSearch(mode){
    const input=document.querySelector('[data-reference-search]');
    const results=document.querySelector('[data-reference-results]');
    const status=document.querySelector('[data-reference-search-status]');
    if(!input||!results||!status) return;

    status.textContent='Загрузка каталога…';
    fetch(CATALOG_URL,{cache:'no-cache'})
      .then(function(response){
        if(!response.ok) throw new Error('catalog_http_'+response.status);
        return response.json();
      })
      .then(function(catalog){
        if(!Array.isArray(catalog)) throw new Error('catalog_format');
        renderResults(catalog,mode,input.value,results,status);
        input.addEventListener('input',function(){
          renderResults(catalog,mode,input.value,results,status);
        });
      })
      .catch(function(){
        status.textContent='Каталог поиска недоступен. Используйте быстрый переход или внутренние ссылки страниц.';
      });
  }

  function boot(){
    const mode=document.documentElement.getAttribute('data-reference-mode');
    setMode(mode);
    normalizeModeLinks(document);
    wrapTables(document.querySelector('.cm-reference-content')||document);
    initCatalogSearch(mode);
  }

  if(document.readyState==='loading') document.addEventListener('DOMContentLoaded',boot); else boot();
})();
