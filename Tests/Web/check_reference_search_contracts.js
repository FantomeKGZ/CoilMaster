'use strict';

const fs=require('fs');
const vm=require('vm');

const source=fs.readFileSync(
  'firmware/esp32/web/sites/reference/shared/reference.js','utf8');

const context={
  window:{},
  document:{
    readyState:'loading',
    addEventListener:()=>{}
  },
  localStorage:{setItem:()=>{}},
  location:{
    href:'http://coil.local/sites/reference/desktop/',
    search:''
  },
  history:{replaceState:()=>{}},
  URL,
  URLSearchParams,
  console
};
vm.runInNewContext(source,context,{filename:'reference.js'});

const search=context.window.CMReferenceSearch;
if(!search||typeof search.normalizeSearch!=='function'||
   typeof search.rankCatalog!=='function'){
  throw new Error('Reference search test API missing');
}

if(search.normalizeSearch('  4А_160—1500  ')!=='4а 160—1500'){
  throw new Error('Reference query normalization changed');
}

const catalog=[
  {title:'АИР 160 1500 об/мин',path:'AIR/160-1500.html',desktop:true,mobile:true},
  {title:'4А 160 3000 об/мин',path:'4A/160-3000.html',desktop:true,mobile:true},
  {title:'4А 160 1500 об/мин',path:'4A/160-1500.html',desktop:true,mobile:true},
  {title:'4А 160 mobile only',path:'4A/mobile-160.html',desktop:false,mobile:true},
  {title:'Серия 4А',path:'4A.html',desktop:true,mobile:true}
];

const unordered=search.rankCatalog(catalog,'desktop','160 4а');
if(unordered.length!==2||
   unordered[0].path!=='4A/160-1500.html'||
   unordered[1].path!=='4A/160-3000.html'){
  throw new Error('Multi-token reference ranking is not deterministic');
}

const latinPath=search.rankCatalog(catalog,'desktop','4A 160');
if(latinPath.length!==2){
  throw new Error('Reference filename search no longer supports Latin source paths');
}

const mobile=search.rankCatalog(catalog,'mobile','mobile 160');
if(mobile.length!==1||mobile[0].path!=='4A/mobile-160.html'){
  throw new Error('Reference search mode availability filter failed');
}

if(!source.includes('history.replaceState')||
   !source.includes("new URLSearchParams(location.search).get('q')")){
  throw new Error('Reference search query URL persistence missing');
}
if(!source.includes("event.key==='Enter'")||
   !source.includes("event.key==='Escape'")||
   !source.includes("clear.addEventListener('click',clearSearch)")){
  throw new Error('Reference keyboard/clear search controls missing');
}
if(!source.includes("querySelectorAll('[data-reference-query]')")||
   !source.includes("button.getAttribute('data-reference-query')")||
   !source.includes("'aria-pressed'")){
  throw new Error('Reference quick series filter behavior missing');
}
if(!source.includes("fetch('/web-bundle-manifest.json'")||
   !source.includes("node.textContent='SD '+commit.slice(0,8)")||
   !source.includes("node.textContent='SD web unknown'")){
  throw new Error('Reference SD bundle provenance badge missing');
}
for(const contract of [
  "image.loading='lazy'",
  "image.decoding='async'",
  "image.alt='Иллюстрация: '+name",
  "wrap.setAttribute('role','region')",
  "wrap.setAttribute('aria-label','Прокручиваемая таблица')",
  'wrap.tabIndex=0'
]){
  if(!source.includes(contract)){
    throw new Error('Reference legacy media/table accessibility missing: '+contract);
  }
}

console.log('Reference catalog search contracts: OK');
