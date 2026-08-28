(() => {
'use strict';
const originalFetch = window.fetch.bind(window);
window.fetch = async function(input, init) {
  const requestUrl = typeof input === 'string' ? input : input && input.url;
  const method = String((init && init.method) || (input && input.method) || 'GET').toUpperCase();
  if (!requestUrl || method !== 'GET') return originalFetch(input, init);

  const url = new URL(requestUrl, location.origin);
  if (url.pathname !== '/api/status') return originalFetch(input, init);

  const response = await originalFetch(input, init);
  if (!response.ok) return response;

  let body;
  try { body = await response.clone().json(); }
  catch (_) { return response; }

  const target = Number(body.repeat_target);
  const completed = Number(body.completed_runs);
  if (body.job_status !== 'PROGRAM_COMPLETED' ||
      !Number.isFinite(target) || target <= 0 ||
      !Number.isFinite(completed) || completed < target) {
    return response;
  }

  const display = {
    ...body,
    last_completed_job_id:body.job_id,
    last_completed_session_id:body.session_id,
    last_completed_run_id:body.last_run_id,
    current_job_cleared_after_completion:true,
    job_id:0,
    session_id:0,
    job_status:'IDLE',
    machine_status:'Ожидание',
    job_type:'WORKING',
    program:'',
    repeat_target:1,
    completed_runs:0,
    linked:false,
    repair_id:null,
    motor_id:null,
    spool_id:null,
    spool_wire_type:null,
    spool_diameter_hundredths_mm:null,
    spool_weight_at_selection_g:null,
    run_active:false,
    arduino_ack_pending:false,
    arduino_cancel_pending:false
  };
  return new Response(JSON.stringify(display), {
    status:response.status,
    statusText:response.statusText,
    headers:{'Content-Type':'application/json; charset=utf-8'}
  });
};
})();
