const fs=require('fs');
const read=p=>fs.readFileSync(p,'utf8');
const desktop=read('firmware/esp32/web/desktop/writeoff.html');
const mobile=read('firmware/esp32/web/mobile/writeoff.html');
const prefetch=read('firmware/esp32/web/shared/material-request-status-prefetch.js');
const runWire=read('firmware/esp32/web/shared/writeoff-spool-suggestion.js');
const web=read('firmware/esp32/src/CM_MaterialRequestWeb.cpp');
const header=read('firmware/esp32/src/CM_MaterialRequestStatusStore.h');
const store=read('firmware/esp32/src/CM_MaterialRequestStatusStore.cpp');
const must=(s,t,l)=>{if(!s.includes(t))throw new Error(`missing ${l}: ${t}`)};
const mustNot=(s,t,l)=>{if(s.includes(t))throw new Error(`forbidden ${l}: ${t}`)};

for(const [label,page] of [['desktop',desktop],['mobile',mobile]]){
  const p=page.indexOf('/shared/material-request-status-prefetch.js');
  const w=page.indexOf('/shared/writeoff-spool-suggestion.js');
  if(p<0||w<0||p>=w)throw new Error(`${label}: status prefetch must load before RUN_WIRE controller`);
}

must(header,'static constexpr uint8_t MaxBatchSize = 24U;','fixed status batch bound');
must(header,'bool resolveBatch(const uint32_t* materialRequestIds,','bounded status resolver');
must(web,'/api/material-requests/status-batch','status batch endpoint');
must(web,'m_statuses.resolveBatch(ids, count, states, found)','batch endpoint authoritative resolver');
must(prefetch,"url.pathname==='/api/material-requests'",'bounded request page interception');
must(prefetch,"'/api/material-requests/status-batch?ids='+encodeURIComponent(ids.join(','))",'one batch status read per page');
must(prefetch,"url.pathname==='/api/material-requests/status'",'legacy per-item status cache compatibility');
must(prefetch,'statusCache.has(id)','local status cache lookup');
must(prefetch,'rows.length>24','client batch bound');
must(prefetch,'seen.has(id)','duplicate page identity fail closed');
must(prefetch,"['DRAFT','ISSUED','PRICED','CLOSED'].includes(item.status)",'status domain validation');
mustNot(prefetch,'/api/material-requests/warehouse','prefetch must not mutate warehouse');
mustNot(prefetch,"method:'POST'",'prefetch must not POST');

const batchStart=store.indexOf('bool MaterialRequestStatusStore::resolveBatch(');
const transitionStart=store.indexOf('bool MaterialRequestStatusStore::transition(',batchStart);
if(batchStart<0||transitionStart<=batchStart)throw new Error('batch resolver body missing');
const batch=store.slice(batchStart,transitionStart);
if((batch.match(/m_storage\.open\(RequestsPath, FILE_READ\)/g)||[]).length!==1)throw new Error('batch request journal must be scanned once');
if((batch.match(/m_storage\.open\(Path, FILE_READ\)/g)||[]).length!==1)throw new Error('batch status journal must be scanned once');
must(batch,'transitionId <= previousTransitionId','global transition ordering');
must(batch,'!found[i] || fromStatus != states[i].status ||','per-request chain fail closed');
mustNot(batch,'std::vector','unbounded ESP32 batch storage');
mustNot(batch,'readString()','whole-file batch buffering');

must(runWire,"confirmed:'true'",'explicit RUN_WIRE confirmation');
must(runWire,"source_kind:'RUN_WIRE'",'RUN_WIRE source kind');
must(runWire,'source_session_id:sourceSessionId','exact source session');
must(runWire,'source_run_id:sourceRunId','exact source run');
must(runWire,'spool_id:String(activeSpool.spool_id)','exact immutable spool');
must(runWire,'/api/material-requests/warehouse','dedicated RUN_WIRE mutation endpoint');
mustNot(runWire,'/api/materials/usage','generic material mutation forbidden for RUN_WIRE');

console.log('RUN_WIRE material-request status batch contracts OK: bounded status prefetch removes server N+1 scans while exact manual RUN_WIRE provenance/mutation semantics remain unchanged.');
