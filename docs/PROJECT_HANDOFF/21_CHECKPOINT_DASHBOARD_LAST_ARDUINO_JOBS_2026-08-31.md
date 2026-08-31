# Checkpoint — dashboard last Arduino jobs — 2026-08-31

Branch: `arduino-ru-lcd-experiment`

## Goal

Replace the now-low-value empty `Текущее задание` presentation on the home page with useful operator context while preserving the existing live runtime monitor.

## Implemented

`firmware/esp32/web/shared/completed-job-display-reset.js` now provides read-only dashboard presentation for:

- `Последнее отправленное на Arduino`
- `Последнее выполненное на Arduino`

Both desktop and mobile already load this shared helper, so no duplicated page-specific implementation was added.

The helper observes the raw `/api/status` response before the existing completed-job visual reset. For an active/latest Web job it can show job id, WORKING/STARTING, program, repeat target and linked/unlinked context. When a job reaches `PROGRAM_COMPLETED`, the same raw status is captured before the UI clears the completed current job.

The latest completed display snapshot is cached in browser `localStorage` only to keep it visible while a newer job is active or after ordinary page refreshes.

## Important provenance boundary

The browser cache is display-only and is not production evidence.

Authoritative evidence remains in CoilMaster persistence / winding journal / job state and Arduino archive. The new UI does not create, edit, delete or replace RUN evidence.

No new backend endpoint or storage domain was added.

## Safety invariants unchanged

- no automatic physical START;
- ESP32/Web does not control SSR;
- no automatic resume after reboot;
- `RUN_COMPLETED` does not perform automatic wire writeoff;
- linked material provenance remains unchanged;
- browser-local dashboard history cannot authorize production or writeoff actions.

## Commits

- `c87b08b7886a4a2936ee3e399911de96622bc2a4` — retain last sent/completed display in shared completed-job helper
- `2f3a523002fa3088a844978437b7de6751f73753` — remove superseded unused helper draft
- `b871278efa8e1812d95c86458e294149635573a4` — dashboard history regression contract
- `e5d3c4ecdbfec6001ca2fe329dbd169fad897f2d` — include dashboard history audit in existing CMP test path

Exact CI must be checked on the exact implementation/test HEAD before calling this checkpoint GREEN.
