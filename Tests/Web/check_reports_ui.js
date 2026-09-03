const fs = require('fs');

const desktop = fs.readFileSync('firmware/esp32/web/desktop/reports.html', 'utf8');
const mobile = fs.readFileSync('firmware/esp32/web/mobile/reports.html', 'utf8');

const failures = [];
function must(source, token, label) {
  if (!source.includes(token)) failures.push(`${label}: missing ${token}`);
}
function mustNot(source, token, label) {
  if (source.includes(token)) failures.push(`${label}: forbidden ${token}`);
}

for (const [label, source] of [['desktop reports', desktop], ['mobile reports', mobile]]) {
  must(source, "/api/repairs?status=CLOSED&cursor='+cursor+'&limit=32", `${label} closed-only bounded registry query`);
  must(source, "(x.current_status||x.status)==='CLOSED'", `${label} defensive CLOSED verification`);
  must(source, "String(x.closed_at||'').startsWith(month)", `${label} selected month verification`);
  must(source, "if(j.has_more!==true)break", `${label} bounded cursor termination`);
  must(source, "!Number.isSafeInteger(next)||next<=cursor", `${label} monotonic cursor guard`);
  must(source, "if(++pages>10000)throw new Error('repairs_page_limit')", `${label} page loop safety bound`);
  must(source, "/api/repairs/costing?repair_id=", `${label} authoritative costing lookup`);
  must(source, "/api/clients/by-id", `${label} authoritative client lookup`);
  must(source, "/api/motors/by-id", `${label} authoritative motor lookup`);
  mustNot(source, "fetch('/api/repairs?cursor='+cursor+'&limit=32", `${label} unfiltered repair registry scan`);
}

if (failures.length) {
  console.error(failures.join('\n'));
  process.exit(1);
}

console.log('Reports UI contract OK: desktop/mobile read only CLOSED repairs with bounded cursor paging, verify month/status defensively, and preserve authoritative costing/client/motor lookups.');
