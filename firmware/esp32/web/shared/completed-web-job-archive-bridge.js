(() => {
'use strict';
const originalFetch = window.fetch.bind(window);
const cursorStates = new Map();
const sourceByExactRun = new Map();
let nextToken = 1;

function exactKey(sessionId, runId) {
  return `${Number(sessionId)}:${Number(runId)}`;
}
function parseProgram(text) {
  const values = String(text || '').trim().split(/[\/,;]/).filter(Boolean).map(Number);
  return values.length && values.every(v => Number.isInteger(v) && v > 0) ? values : null;
}
function programMatches(candidate, query, tolerance) {
  if (!query) return true;
  const left = parseProgram(candidate);
  const right = parseProgram(query);
  if (!left || !right || left.length !== right.length) return false;
  return left.every((value, index) =>
    Math.abs(value - right[index]) * 100 <= right[index] * tolerance);
}
async function responseJson(response) {
  let body = {};
  try { body = await response.json(); } catch (_) {}
  if (!response.ok) {
    return {ok:false, status:response.status, body};
  }
  return {ok:true, status:response.status, body};
}
function jsonResponse(status, body) {
  return new Response(JSON.stringify(body), {
    status,
    headers:{'Content-Type':'application/json; charset=utf-8'}
  });
}
function rememberSource(item, source) {
  const key = exactKey(item.session_id, item.run_id);
  const previous = sourceByExactRun.get(key);
  if (previous && previous !== source)
    throw new Error('archive_exact_run_source_collision');
  sourceByExactRun.set(key, source);
}
async function fetchLocalPage(state, limit, program, tolerance) {
  if (state.localDone) return {items:[], has_more:false, next_cursor:null};
  const query = new URLSearchParams({limit:String(limit), tolerance_percent:String(tolerance)});
  if (program) query.set('program', program);
  if (state.localCursor) query.set('cursor', String(state.localCursor));
  const result = await responseJson(await originalFetch('/api/autonomous-windings?' + query, {cache:'no-store'}));
  if (!result.ok) throw Object.assign(new Error(result.body.error || 'local_archive_failed'), {status:result.status});
  return result.body;
}
async function fetchWebPage(state, limit) {
  if (state.webDone) return {items:[], has_more:false, next_cursor:null};
  const query = new URLSearchParams({limit:String(limit)});
  if (state.webCursor) query.set('cursor', String(state.webCursor));
  const result = await responseJson(await originalFetch('/api/autonomous-windings/web-completed?' + query, {cache:'no-store'}));
  if (!result.ok) throw Object.assign(new Error(result.body.error || 'web_archive_failed'), {status:result.status});
  return result.body;
}

window.fetch = async function(input, init) {
  const requestUrl = typeof input === 'string' ? input : input && input.url;
  if (!requestUrl) return originalFetch(input, init);

  const url = new URL(requestUrl, location.origin);
  const method = String((init && init.method) || (input && input.method) || 'GET').toUpperCase();

  if (method === 'GET' && url.pathname === '/api/autonomous-windings') {
    const requestedCursor = Number(url.searchParams.get('cursor') || 0);
    const limit = Math.max(1, Math.min(32, Number(url.searchParams.get('limit') || 20)));
    const tolerance = Math.max(0, Math.min(50, Number(url.searchParams.get('tolerance_percent') || 20)));
    const program = url.searchParams.get('program') || '';

    let state;
    if (requestedCursor === 0) {
      state = {localCursor:0, webCursor:0, localDone:false, webDone:false, program, tolerance};
    } else {
      state = cursorStates.get(requestedCursor);
      if (!state || state.program !== program || state.tolerance !== tolerance)
        return jsonResponse(400, {error:'invalid_combined_archive_cursor'});
    }

    try {
      const [localPage, webPage] = await Promise.all([
        fetchLocalPage(state, limit, program, tolerance),
        fetchWebPage(state, limit)
      ]);
      const localItems = (localPage.items || []).map(item => ({...item, source:item.source || 'ARDUINO_LOCAL'}));
      const webItems = (webPage.items || [])
        .filter(item => programMatches(item.program, program, tolerance))
        .map(item => ({...item, source:'ESP32_JOB'}));

      for (const item of localItems) rememberSource(item, 'ARDUINO_LOCAL');
      for (const item of webItems) rememberSource(item, 'ESP32_JOB');

      const localMore = localPage.has_more === true;
      const webMore = webPage.has_more === true;
      const hasMore = localMore || webMore;
      let nextCursor = null;
      if (hasMore) {
        nextCursor = nextToken++;
        if (nextToken > 0x7fffffff) nextToken = 1;
        cursorStates.set(nextCursor, {
          localCursor:localMore ? Number(localPage.next_cursor) : state.localCursor,
          webCursor:webMore ? Number(webPage.next_cursor) : state.webCursor,
          localDone:!localMore,
          webDone:!webMore,
          program,
          tolerance
        });
      }

      return jsonResponse(200, {
        items:[...localItems, ...webItems],
        count:localItems.length + webItems.length,
        cursor:requestedCursor,
        limit,
        has_more:hasMore,
        next_cursor:nextCursor,
        tolerance_percent:tolerance,
        max_page_size:32,
        sources:['ARDUINO_LOCAL','ESP32_JOB']
      });
    } catch (error) {
      return jsonResponse(error.status || 500, {error:error.message || 'combined_archive_failed'});
    }
  }

  if (method === 'POST' && url.pathname === '/api/autonomous-windings/assign') {
    let body = init && init.body;
    if (!(body instanceof URLSearchParams))
      return originalFetch(input, init);
    const key = exactKey(body.get('session_id'), body.get('run_id'));
    const source = sourceByExactRun.get(key);
    if (source === 'ESP32_JOB') {
      return originalFetch('/api/autonomous-windings/web-completed/assign', init);
    }
  }

  return originalFetch(input, init);
};
})();
