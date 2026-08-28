(()=>{
'use strict';
const nativeFetch=globalThis.fetch.bind(globalThis);
const statusCache=new Map();
const jsonResponse=(payload,status=200)=>new Response(JSON.stringify(payload),{status,headers:{'Content-Type':'application/json; charset=utf-8'}});
const parsedUrl=input=>{try{return new URL(typeof input==='string'?input:String(input&&input.url||''),location.origin)}catch(_){return null}};
const methodOf=init=>String(init&&init.method||'GET').toUpperCase();

globalThis.fetch=async function(input,init){
    const url=parsedUrl(input);
    const method=methodOf(init);
    if(url&&url.origin===location.origin&&method==='GET'&&url.pathname==='/api/material-requests/status'){
        const id=url.searchParams.get('material_request_id')||'';
        if(statusCache.has(id))return jsonResponse(statusCache.get(id));
    }

    const response=await nativeFetch(input,init);
    if(!url||url.origin!==location.origin||method!=='GET'||url.pathname!=='/api/material-requests'||!response.ok)return response;

    let page;
    try{page=await response.clone().json()}catch(_){return jsonResponse({error:'material_request_list_invalid_json'},500)}
    const rows=Array.isArray(page.items)?page.items:[];
    if(rows.length===0)return response;
    if(rows.length>24)return jsonResponse({error:'material_request_status_batch_too_large'},500);

    const ids=[];
    const seen=new Set();
    for(const row of rows){
        const id=String(row&&row.material_request_id||'');
        if(!/^[1-9]\d*$/.test(id)||seen.has(id))return jsonResponse({error:'material_request_status_batch_identity_invalid'},500);
        seen.add(id);ids.push(id);
    }

    const batchResponse=await nativeFetch('/api/material-requests/status-batch?ids='+encodeURIComponent(ids.join(',')),{cache:'no-store'});
    let batch;
    try{batch=await batchResponse.json()}catch(_){return jsonResponse({error:'material_request_status_batch_invalid_json'},500)}
    if(!batchResponse.ok)return jsonResponse({error:batch.error||'material_request_status_batch_read_failed'},batchResponse.status||500);
    const statuses=Array.isArray(batch.items)?batch.items:[];
    if(statuses.length!==ids.length)return jsonResponse({error:'material_request_status_batch_count_mismatch'},500);

    for(let i=0;i<ids.length;i++){
        const item=statuses[i];
        if(!item||String(item.material_request_id)!==ids[i]||item.found!==true||
           !['DRAFT','ISSUED','PRICED','CLOSED'].includes(item.status)||
           !Number.isInteger(Number(item.transition_count))||Number(item.transition_count)<0){
            return jsonResponse({error:'material_request_status_batch_identity_mismatch'},500);
        }
        statusCache.set(ids[i],{
            material_request_id:Number(ids[i]),
            status:item.status,
            transition_count:Number(item.transition_count)
        });
    }
    return response;
};
})();
