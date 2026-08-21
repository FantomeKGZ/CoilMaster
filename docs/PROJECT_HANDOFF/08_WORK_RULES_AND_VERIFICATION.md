# Правила разработки и проверки

Дата актуализации: **2026-08-21**

## Source of truth

Рабочая ветка:

```text
cmp-protocol-v1
```

`main` не использовать как источник реализации.

Перед изменением существующего файла:

1. fetch текущего файла из `cmp-protocol-v1`;
2. использовать current blob SHA;
3. не переиспользовать старый SHA после другого commit;
4. при конфликте повторно fetch и аккуратно объединить изменения.

Для нового файла сначала проверить, что exact path отсутствует.

## Как выбирать работу

Активную задачу определяют только:

```text
docs/PROJECT_HANDOFF/00_READ_FIRST.md
docs/PROJECT_HANDOFF/61_CURRENT_RECOVERY_AND_DOCS_BASELINE_2026-08-21.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
current concrete failure
explicit user request
```

Старые numbered checkpoints — historical evidence. Их `next`/`pending` не являются текущей очередью.

## Основной цикл изменения

```text
1. Confirm branch/head.
2. Read current owner + contract + persistence + UI/API + verification path.
3. Fetch exact files/current blob SHA.
4. Make the smallest coherent change.
5. Update targeted regression contract when semantics changed.
6. Run applicable automated gate(s).
7. Run targeted hardware gate only when affected scope requires it.
8. Update current docs only when project state materially changed.
```

## Verification labels

```text
NOT VERIFIED     gate unavailable/not run
FAILED           named gate ran and failed
GREEN/SUCCESS    named gate actually completed successfully
USER CONFIRMED   user explicitly verified real hardware behavior
APPROVED         architecture/contract decision accepted
```

A successful commit is never proof of build/test success.

## Current verified production baseline

On exact production commit:

```text
e35c4bfe0cef3c2342ad6b27e43cc931fe14dd00
```

confirmed:

```text
CMP Protocol Tests #2175 — GREEN
ESP32 Build #1241 — GREEN
```

Arduino Uno Build for the current documentation/recovery head and current-head hardware acceptance remain separate gates.

## C++ review checklist

Check applicable items:

- `.h`/`.cpp` signatures match;
- no duplicate definitions across separately compiled `.cpp` files;
- namespace/type/include dependencies are correct;
- no unsupported SDK call;
- fixed buffers are bounded;
- no accidental large Uno stack/SRAM regression;
- integer conversions/overflow are correct;
- SD/files close on all paths;
- mutation happens only after validation/preflight;
- new persisted files are included in integrity/backup/restore coverage.

The 2026-08-21 ESP32 linker failure is a concrete example: duplicate job-lifecycle definitions compiled cleanly individually but failed at link time. Source audits must therefore include duplicate-definition/topology checks.

## Automated workflows

```text
.github/workflows/cmp-protocol-tests.yml
.github/workflows/esp32-build.yml
.github/workflows/arduino-uno-build.yml
```

Run the affected workflow(s), not a generic historical suite.

The CMP Protocol workflow is configured to continue later Web/safety audits after one audit fails so a single run exposes all remaining contract failures. Any failed audit still fails the job.

## UART/protocol changes

Any production `CMP1|...` contract change requires:

- Arduino side review;
- ESP32 side review;
- CRC/length/range checks;
- retry/timeout behavior;
- staged compatibility decision where applicable;
- CMP Protocol Tests;
- both board builds if peer code changed;
- targeted two-board hardware verification when wire behavior changed.

Remote acceptance/cancel/event ACK must never become physical START permission.

## JOB cancel/recovery

This block is currently implemented and closed at repo level:

```text
no-run cancel
idempotent ALREADY_CLEAR
safe D -> * -> # -> D fallback
ALL_CLEAR != RUN_COMPLETED
active physical run cannot be cleared
reboot != auto-start/auto-complete/auto-writeoff
```

Do not reopen it because an old checkpoint lists it as pending. Re-test/rework only after a concrete regression or a change touching this boundary.

## Safety rules

Never accept a change that can:

- start movement remotely/automatically;
- auto-start a repeat run;
- allow ESP32/Web to drive SSR directly;
- leave SSR enabled after fault/abort;
- treat lost communication as permission;
- auto-resume after reboot;
- synthesize `RUN_COMPLETED` from recovery/cancel;
- auto-writeoff material from `RUN_COMPLETED`;
- write off run-linked material without exact `source_session_id + source_run_id`;
- lose exact spool provenance when a spool is actually used;
- auto-apply restore or delete production data automatically.

`spool_id` may be absent only in the approved KG_FIRST unallocated/manual consumption path.

## Data/persistence rules

Before mutation validate applicable identity, repair/material/spool references, quantity/currency/range and source run evidence. Multi-step mutations use explicit pending/recovery semantics where required.

Historical cost/price/material evidence is persisted; UI must not recompute old operations from current values.

New production `/data` files are incomplete until integrity audit + backup/export + restore/rollback handling are defined.

## UI rules

Meaningful operator functionality should maintain desktop/mobile parity unless a deliberate exception is documented.

UI must show server errors, remain subordinate to server truth, expose UNKNOWN/corruption safely and never imply physical motion before confirmed hardware evidence.

Dynamic UI markup should be tested in the source that actually generates it (for example shared JS), not incorrectly required in static HTML.

## Documentation rules

When a contract materially changes:

- update the owning thematic doc;
- update `docs/AI_AGENT/` router/map when ownership/location changes;
- update `00_READ_FIRST.md` / current checkpoint when project status changes;
- keep older checkpoints historical rather than rewriting their original evidence.

`05_COMPLETED_WORK_LOG.md` and `10_SESSION_LOG.md` are history/logs, not active task selectors.

## Definition of Done

Applicable checks must be explicit:

```text
[ ] code on cmp-protocol-v1
[ ] current blob SHAs used
[ ] safety contract preserved
[ ] relevant regression audit updated
[ ] applicable named workflow GREEN or explicitly NOT VERIFIED
[ ] targeted hardware verification passed if required
[ ] current docs updated if state changed
[ ] no old checkpoint was accidentally promoted into active work
```
