(() => {
'use strict';

const host = document.getElementById('items');
if (!host) return;

function exactText(text) {
  const match = String(text || '').match(/(\d+)\s*\/\s*(\d+)/);
  return match ? {session:match[1], run:match[2], text:`${match[1]} / ${match[2]}`} : null;
}

function appendExact(container, exact) {
  if (!container || container.dataset.exactRunAdded === '1') return;
  container.dataset.exactRunAdded = '1';
  const line = document.createElement('span');
  line.className = 'cm-exact-run-id';
  line.textContent = `Session / Run: ${exact.text}`;
  if (container.classList.contains('tip-box')) {
    container.appendChild(document.createElement('br'));
    container.appendChild(line);
  } else {
    const p = document.createElement('p');
    p.className = 'mini cm-exact-run-id';
    p.textContent = `Session / Run: ${exact.text}`;
    container.appendChild(p);
  }
}

function compactDesktop() {
  const table = host.querySelector('table');
  if (!table) return false;
  const heading = table.querySelector('thead th:nth-child(2)');
  if (heading) {
    heading.textContent = '№';
    heading.title = 'Порядковый номер в текущем списке. Exact Session / Run находится в Info.';
  }
  const rows = table.querySelectorAll('tbody tr');
  let ordinal = 0;
  rows.forEach(row => {
    const cells = row.querySelectorAll('td');
    if (cells.length < 2) return;
    const exact = exactText(cells[1].textContent);
    if (!exact) return;
    ordinal += 1;
    cells[1].dataset.exactSessionId = exact.session;
    cells[1].dataset.exactRunId = exact.run;
    cells[1].title = `Exact Session / Run: ${exact.text}`;
    cells[1].innerHTML = `<b>№${ordinal}</b>`;
    appendExact(row.querySelector('.tip-box'), exact);
  });
  return true;
}

function compactMobile() {
  const cards = host.querySelectorAll('article.task');
  if (!cards.length) return false;
  let ordinal = 0;
  cards.forEach(card => {
    const rows = card.querySelectorAll('.row');
    let exact = null;
    rows.forEach(row => {
      const label = row.querySelector('.muted');
      if (!label || label.textContent.trim() !== 'Session / Run') return;
      const value = row.querySelector('b');
      exact = exactText(value ? value.textContent : '');
      if (!exact || !value) return;
      ordinal += 1;
      label.textContent = '№';
      value.dataset.exactSessionId = exact.session;
      value.dataset.exactRunId = exact.run;
      value.title = `Exact Session / Run: ${exact.text}`;
      value.textContent = `№${ordinal}`;
    });
    if (exact) appendExact(card.querySelector('details'), exact);
  });
  return true;
}

function apply() {
  if (document.body.dataset.view === 'mobile') compactMobile();
  else compactDesktop();
}

let queued = false;
function schedule() {
  if (queued) return;
  queued = true;
  Promise.resolve().then(() => {
    queued = false;
    apply();
  });
}

new MutationObserver(schedule).observe(host, {childList:true, subtree:true});
schedule();
})();
