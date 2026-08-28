(()=>{
'use strict';
const nativeFetch=globalThis.fetch.bind(globalThis);
let selectedSpoolId='';
const parsedUrl=input=>{try{return new URL(typeof input==='string'?input:String(input&&input.url||''),location.origin)}catch(_){return null}};
const methodOf=(input,init)=>String(init&&init.method||(input&&input.method)||'GET').toUpperCase();
const jsonResponse=(payload,status=200)=>new Response(JSON.stringify(payload),{status,headers:{'Content-Type':'application/json; charset=utf-8'}});

globalThis.fetch=async function(input,init){
  const url=parsedUrl(input);
  const method=methodOf(input,init);

  if(url&&url.origin===location.origin&&method==='GET'&&url.pathname==='/api/warehouse/spools'&&
     url.searchParams.get('material')==='ALL'&&url.searchParams.get('limit')==='32'&&selectedSpoolId){
    const id=selectedSpoolId;
    selectedSpoolId='';
    const exact=await nativeFetch('/api/warehouse/spools/by-id?spool_id='+encodeURIComponent(id),{cache:'no-store'});
    let item={};
    try{item=await exact.json()}catch(_){return jsonResponse({error:'active_spool_exact_invalid_json'},500)}
    if(exact.status===404)return jsonResponse({items:[],has_more:false,next_cursor:null,total_count:0,count:0,returned_count:0});
    if(!exact.ok)return jsonResponse({error:item.error||'active_spool_exact_read_failed'},exact.status||500);
    if(String(item.spool_id)!==id||item.status!=='ACTIVE'||
       !['CU','AL'].includes(item.material_class)||
       !Number.isInteger(Number(item.diameter_hundredths_mm))||Number(item.diameter_hundredths_mm)<=0||
       !Number.isInteger(Number(item.current_weight_g))||Number(item.current_weight_g)<=0){
      return jsonResponse({error:'active_spool_exact_identity_mismatch'},500);
    }
    return jsonResponse({items:[item],has_more:false,next_cursor:null,total_count:1,count:1,returned_count:1});
  }

  const response=await nativeFetch(input,init);
  if(!url||url.origin!==location.origin||method!=='GET'||url.pathname!=='/api/jobs/spool-selection'||!response.ok)return response;
  let selection={};
  try{selection=await response.clone().json()}catch(_){return jsonResponse({error:'spool_selection_invalid_response'},500)}
  const id=String(selection&&selection.spool_id||'');
  if(!/^[1-9]\d*$/.test(id))return jsonResponse({error:'spool_selection_identity_mismatch'},500);
  selectedSpoolId=id;
  return response;
};
})();
