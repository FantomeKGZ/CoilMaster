# Активная работа и следующие шаги

Дата обновления: **2026-08-25**  
Ветка: **`cmp-protocol-v1`**

Этот файл содержит только текущую активную очередь. Старые checkpoints — history/evidence, а не backlog.

## Current verified baseline

Latest fully verified block: **89_REPAIR_PRICING_REFERENCE_BATCHING**.

```text
ESP32 Build #1441 / run 32818211915 / SUCCESS
CMP #3096 / run 32818305639 / SUCCESS
final descendant 921999a8f2a11405c8a312a4f6064c2a29834e93
```

Overall project readiness: **~95%**. Software/repo implementation and integrity: **~98-99%**.

## Active phase

Основной production flow, safety contracts, Hall split, Uno runtime, persistence, Web, backup/restore, materials/warehouse/costing and reference-site integration собраны.

Текущая работа — узкий **Stage-1 ESP32/storage performance review** перед финальным hardware acceptance.

Искать только доказанные кандидаты:

1. per-record full-file reference scans;
2. duplicate authoritative scans с эквивалентной семантикой;
3. измеримое avoidable I/O/allocation без ослабления exact uniqueness/fail-closed semantics.

Если повторный scan нужен для отдельной семантики или его удаление требует unbounded RAM/indexing — `KEEP`.

## Recently closed GREEN blocks

```text
80  writeoff single-pass movement lookup
81  winding completion single-pass
82  finalization winding single-pass
83  material backup scoped audit
84  warehouse backup scoped audit
85  NDJSON performance/rotation strategy
86  workshop winding single-pass
87  finalization costing/movement single-pass
88  material reference batching
89  repair pricing reference batching
```

## Current KEEP

Do not reopen without new evidence:

- repair-status bounded self-scan;
- autonomous assignment event batching;
- warehouse movement provenance uniqueness batching;
- legacy ESP32 `CAL_RESULT` receive fallback;
- Uno resource-sensitive buffers/contracts already classified KEEP.

## NDJSON rule

- no premature DB migration;
- no automatic cleanup/rotation;
- no production-data truncation;
- `/api/system/storage` observability is available;
- threshold/rotation decisions only after real-device measurements.

## Safety invariants

Never weaken:

- no automatic physical START;
- no automatic START between repeats;
- no auto-resume after reboot;
- Arduino owns SSR;
- ESP32/Web never controls SSR directly;
- lost ACK/timeout never proves Arduino idle;
- final repeat cannot auto-reopen;
- RUN_COMPLETED never auto-writes off wire/material;
- linked-production writeoff remains manual and exact `source_session_id + source_run_id + immutable spool_id`;
- cancellation never erases immutable run/history evidence;
- restore is operator-only, transactional, fail-closed.

## Next steps

1. Continue narrow repo-only Stage-1 audit and implement only justified hotspots with regression + CI gate + handoff.
2. When no justified candidate remains, stop software optimization instead of forcing rewrites.
3. Then perform one full two-board ESP32 + Arduino Uno hardware acceptance:
   - boot/handshake;
   - JOB delivery without auto-start;
   - physical START;
   - RUN_STARTED/RUN_COMPLETED;
   - Hall count;
   - repeat requiring another physical START;
   - cancel/recovery;
   - reboot no auto-resume;
   - manual exact-run exact-spool writeoff;
   - Hall calibration ARM/local confirm/physical start/CAL_DONE/proposal/local apply/reconciliation;
   - keypad/LCD/buzzer;
   - SSR owned only by Uno.
4. After hardware acceptance, use real device storage metrics to decide whether any NDJSON rotation thresholds are actually needed.

## New-chat entrypoint

Read first:

```text
/AGENTS.md
docs/PROJECT_HANDOFF/00_READ_FIRST.md
docs/PROJECT_HANDOFF/90_PROJECT_COMPLETION_AND_NEXT_CHAT_2026-08-25.md
```

`90_PROJECT_COMPLETION_AND_NEXT_CHAT_2026-08-25.md` contains the current completion estimate, latest GREEN evidence, safety/architecture summary, KEEP decisions, hardware gate and ready-to-paste new-chat prompt.
