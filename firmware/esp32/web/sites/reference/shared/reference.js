(function(){
  const MODE_KEY='cm-ui-version';
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
  function boot(){
    const mode=document.documentElement.getAttribute('data-reference-mode');
    setMode(mode);
    normalizeModeLinks(document);
    wrapTables(document.querySelector('.cm-reference-content')||document);
  }
  if(document.readyState==='loading') document.addEventListener('DOMContentLoaded',boot); else boot();
})();
