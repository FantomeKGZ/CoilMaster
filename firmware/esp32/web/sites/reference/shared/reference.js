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
      wrap.tabIndex=0;
      wrap.setAttribute('role','region');
      wrap.setAttribute('aria-label','Прокручиваемая таблица');
      table.parentNode.insertBefore(wrap,table);
      wrap.appendChild(table);
    });
  }

  function enhanceImages(root){
    root.querySelectorAll('img').forEach(function(image){
      image.loading='lazy';
      image.decoding='async';
      if(!image.hasAttribute('alt')){
        let name='изображение';
        try{
          const path=new URL(image.src,location.href).pathname;
          name=decodeURIComponent(path.split('/').pop()||name);
        }catch(_){}
        image.alt='Иллюстрация: '+name;
      }
    });
  }

  function normalizeModeLinks(root){
    root.querySelectorAll('[data-reference-mode]').forEach(function(link){
      link.addEventListener('click',function(){setMode(link.getAttribute('data-reference-mode'));});
    });
  }

  function normalizeSearch(value){
    return String(value||'')
      .toLocaleLowerCase('ru-RU')
      .replace(/ё/g,'е')
      .replace(/[.,;:()/_-]+/g,' ')
      .replace(/\s+/g,' ')
      .trim();
  }

  function rankCatalog(catalog,mode,query){
    const needle=normalizeSearch(query);
    if(!needle) return [];
    const tokens=needle.split(' ');

    return catalog
      .filter(function(entry){
        if(entry[mode]!==true) return false;
        const title=normalizeSearch(entry.title);
        const path=normalizeSearch(entry.path);
        return tokens.every(function(token){
          return title.includes(token)||path.includes(token);
        });
      })
      .map(function(entry){
        const title=normalizeSearch(entry.title);
        const path=normalizeSearch(entry.path);
        let score=5;
        if(title===needle) score=0;
        else if(path===needle) score=1;
        else if(title.startsWith(needle)) score=2;
        else if(path.startsWith(needle)) score=3;
        else if(title.includes(needle)) score=4;
        return {entry:entry,score:score};
      })
      .sort(function(left,right){
        return left.score-right.score||
          String(left.entry.title).localeCompare(String(right.entry.title),'ru');
      })
      .map(function(item){return item.entry;});
  }

  function searchFromLocation(){
    try{return new URLSearchParams(location.search).get('q')||'';}catch(_){return '';}
  }

  function updateSearchLocation(value){
    if(!history||typeof history.replaceState!=='function') return;
    const url=new URL(location.href);
    const query=String(value||'').trim();
    if(query) url.searchParams.set('q',query); else url.searchParams.delete('q');
    history.replaceState(null,'',url.pathname+url.search+url.hash);
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

    const matches=rankCatalog(catalog,mode,needle);

    status.textContent=matches.length
      ? 'Найдено: '+matches.length+(matches.length>MAX_RESULTS?' · показаны первые '+MAX_RESULTS:'')
      : 'Ничего не найдено. Попробуйте часть марки или названия.';

    matches.slice(0,MAX_RESULTS).forEach(function(entry){
      const link=document.createElement('a');
      link.className='cm-reference-search-result';
      link.setAttribute('role','listitem');
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
    const clear=document.querySelector('[data-reference-search-clear]');
    const quick=document.querySelectorAll('[data-reference-query]');
    const results=document.querySelector('[data-reference-results]');
    const status=document.querySelector('[data-reference-search-status]');
    if(!input||!clear||!results||!status) return;

    if(!input.value) input.value=searchFromLocation();
    const syncClear=function(){clear.hidden=!input.value;};
    const syncQuick=function(){
      const current=normalizeSearch(input.value);
      quick.forEach(function(button){
        button.setAttribute(
          'aria-pressed',
          normalizeSearch(button.getAttribute('data-reference-query'))===current?'true':'false'
        );
      });
    };
    syncClear();
    syncQuick();
    status.textContent='Загрузка каталога…';
    fetch(CATALOG_URL,{cache:'no-cache'})
      .then(function(response){
        if(!response.ok) throw new Error('catalog_http_'+response.status);
        return response.json();
      })
      .then(function(catalog){
        if(!Array.isArray(catalog)) throw new Error('catalog_format');
        renderResults(catalog,mode,input.value,results,status);
        const clearSearch=function(){
          input.value='';
          updateSearchLocation('');
          renderResults(catalog,mode,'',results,status);
          syncClear();
          syncQuick();
          input.focus();
        };
        input.addEventListener('input',function(){
          updateSearchLocation(input.value);
          renderResults(catalog,mode,input.value,results,status);
          syncClear();
          syncQuick();
        });
        input.addEventListener('keydown',function(event){
          if(event.key==='Escape'){
            event.preventDefault();
            clearSearch();
          }else if(event.key==='Enter'){
            const first=results.querySelector('a');
            if(first){
              event.preventDefault();
              location.href=first.href;
            }
          }
        });
        clear.addEventListener('click',clearSearch);
        quick.forEach(function(button){
          button.addEventListener('click',function(){
            input.value=button.getAttribute('data-reference-query')||'';
            updateSearchLocation(input.value);
            renderResults(catalog,mode,input.value,results,status);
            syncClear();
            syncQuick();
            input.focus();
          });
        });
      })
      .catch(function(){
        status.textContent='Каталог поиска недоступен. Используйте быстрый переход или внутренние ссылки страниц.';
      });
  }

  function initBundleProvenance(){
    const host=document.querySelector('.cm-reference-top')||
      document.querySelector('.cm-reference-page-toolbar');
    if(!host) return;
    const node=document.createElement('span');
    node.className='cm-reference-bundle';
    node.textContent='SD web unknown';
    host.appendChild(node);
    fetch('/web-bundle-manifest.json',{cache:'no-store'})
      .then(function(response){
        if(!response.ok) throw new Error('bundle_manifest_http_'+response.status);
        return response.json();
      })
      .then(function(bundle){
        const commit=String(bundle.coilmaster_commit||'');
        if(bundle.schema_version!==1||!(/^[0-9a-f]{40}$/i.test(commit))) return;
        node.textContent='SD '+commit.slice(0,8);
        node.title='Web bundle '+commit+
          ' · legacy '+String(bundle.legacy_commit||'unknown')+
          ' · '+String(bundle.generated_utc||'unknown');
      })
      .catch(function(){});
  }

  window.CMReferenceSearch={
    normalizeSearch:normalizeSearch,
    rankCatalog:rankCatalog
  };

  function boot(){
    const mode=document.documentElement.getAttribute('data-reference-mode');
    setMode(mode);
    normalizeModeLinks(document);
    const content=document.querySelector('.cm-reference-content')||document;
    wrapTables(content);
    enhanceImages(content);
    initCatalogSearch(mode);
    initBundleProvenance();
  }

  if(document.readyState==='loading') document.addEventListener('DOMContentLoaded',boot); else boot();
})();
