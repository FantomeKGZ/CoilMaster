(()=>{
  const unsafeControlIds=[
    'remoteBackupBatch','remoteBackupRetention','remoteBackupInspectId',
    'remoteBackupInspect','remoteBackupStage','remoteBackupRestorePlan',
    'remoteBackupRollbackSnapshot','remoteBackupApplyPreflight',
    'remoteBackupApply'
  ];
  const safeMessage='После перезапуска найдены следы предыдущего применения backup. Автопродолжение запрещено. Runtime-счётчики применения после reboot сброшены и не доказывают, были ли рабочие данные заменены. Сначала вручную проверьте clients, motors, repairs, warehouse и winding; временные/страховочные файлы удаляйте только после подтверждения состояния.';
  let stale=false;

  function enforceStaleUi(){
    if(!stale)return;
    unsafeControlIds.forEach(id=>{
      const el=document.getElementById(id);
      if(el&&!el.disabled)el.disabled=true;
    });
    document.querySelectorAll('[data-remote-backup]').forEach(el=>{
      if(!el.disabled)el.disabled=true;
    });
    const discard=document.getElementById('remoteBackupStageDiscard');
    if(discard&&discard.disabled)discard.disabled=false;
    const state=document.getElementById('remoteBackupState');
    if(state){
      if(state.className!=='warning')state.className='warning';
      if(state.textContent!==safeMessage)state.textContent=safeMessage;
    }
  }

  async function checkApplyEvidence(){
    try{
      const response=await fetch('/api/backup/remote/apply-status',{cache:'no-store'});
      const data=await response.json();
      if(response.ok&&data.state==='STALE'){
        stale=true;
        enforceStaleUi();
      }else if(response.ok&&data.state==='IDLE'){
        stale=false;
      }
    }catch(_){
      // The main backup helper owns normal transport-error reporting.
    }
    setTimeout(checkApplyEvidence,800);
  }

  const observer=new MutationObserver(()=>enforceStaleUi());
  observer.observe(document.body,{childList:true,subtree:true,attributes:true,attributeFilter:['disabled','class']});
  checkApplyEvidence();
})();
