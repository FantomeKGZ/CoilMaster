# Текущее состояние CoilMaster

Дата обновления: **2026-08-21**  
Ветка: **`cmp-protocol-v1`**

Этот файл описывает только **текущее** состояние. Исторические детали находятся в numbered checkpoints и не являются очередью активных работ.

## Source of truth

Единственный источник реализации — `cmp-protocol-v1`. `main` не использовать как источник кода.

Перед изменением existing file fetch текущего содержимого и blob SHA. Не объявлять build/CI/hardware success без фактического результата.

## Current verified repo baseline

Production commit:

```text
e35c4bfe0cef3c2342ad6b27e43cc931fe14dd00
Fix duplicate ESP32 job lifecycle definitions
```

Verified GitHub Actions на exact commit:

```text
CMP Protocol Tests #2175 — GREEN
ESP32 Build #1241 — GREEN
```

Последняя ESP32 build regression была linker duplicate-definition в job lifecycle и исправлена удалением obsolete `CM_JobStateStoreLifecycle.cpp`. Authoritative implementations находятся в `CM_JobStateRemoteCancel.cpp` и `CM_JobStateDismiss.cpp`.

## Architectural ownership

Arduino Uno:

- physical START;
- SSR;
- Hall turn counting;
- keypad/LCD/buzzer;
- realtime winding state machine;
- accepted remote job execution;
- RUN_STARTED/RUN_COMPLETED generation.

ESP32:

- Wi-Fi/AP/HTTP/FTP;
- microSD/RTC;
- workshop registry;
- winding job preparation/persistence;
- warehouse/materials/costing;
- winding journal/archive;
- backup/restore;
- mobile/desktop UI.

Production cross-board protocol: text `CMP1|...` over SoftwareSerial/UART link. `Shared/Protocol/` is older binary host-test code, not production wire protocol.

## Safety boundary

- physical START только local/physical;
- no automatic START between repeats;
- no auto-resume after reboot;
- ESP32/Web never drive SSR directly;
- `RUN_COMPLETED` never auto-writes off material;
- manual writeoff requires exact `source_session_id + source_run_id`;
- `spool_id` optional only for approved KG_FIRST unallocated/manual consumption;
- exact spool provenance remains mandatory whenever a spool is actually used;
- backup restore operator-only, transactional, fail-closed;
- hardware settings and Hall calibration safe-idle gated.

## Current production flow

```text
client -> motor -> OPEN repair -> costing -> linked winding
-> immutable job snapshot/material selection
-> UART delivery -> physical START
-> RUN_STARTED / RUN_COMPLETED
-> manual exact-run material writeoff
-> costing/finalization -> CLOSED -> reports -> backup
```

A repeat target is one JOB containing multiple physical RUNs. Every RUN requires a new physical START.

## JOB cancel / recovery

Implemented and repo-level CLOSED:

- no-run remote JOB cancel;
- Arduino idempotent `ALREADY_CLEAR` response when remote job is absent;
- physical fallback `D -> * -> # -> D`;
- fallback emits `ALL_CLEAR`, never `RUN_COMPLETED`;
- active physical run prevents unsafe clear;
- reboot never auto-starts and never fabricates run completion/writeoff;
- historical/immutable run evidence is not erased by operational cancellation.

Do not reopen this area without a concrete observed regression.

## Material/writeoff model

Current new-consumption model is KG_FIRST:

```text
quantity_kg authoritative
exact source_session_id + source_run_id provenance mandatory
spool optional only for unallocated/manual KG_FIRST path
exact spool decrement only with explicit spool
legacy exact-spool records remain supported
```

Wire/material writeoff remains explicit operator action after a completed run.

## Persistence and integrity

Implemented persisted domains include:

- clients/motors/repairs/status;
- winding job allocator/snapshots/state/spool selection;
- winding event journal;
- warehouse/spools/movements/pricing;
- materials usage/adjustments;
- autonomous Arduino winding archive;
- conductor settings;
- backup/restore metadata.

Backup deep validation is read-only and fail-closed. Session persistence preflight is owned by `WindingSessionPersistenceIntegrityAudit`; duplicate full session-directory preflight in backup export was removed.

## UI/API state

Desktop and mobile interfaces cover the implemented workshop, motor/import, repairs, linked winding, Arduino archive, materials/warehouse, costing, reports, settings, backup/network and diagnostics flows. Growing collections use bounded/paged APIs where implemented; old checkpoints describing their migration are historical, not active work.

## Verification still separate

The following are **not implied** by current green Protocol + ESP32 workflows:

```text
Arduino Uno Build current HEAD
two-board UART hardware smoke current HEAD
full hardware acceptance current HEAD
```

Historical real-device checks remain valid evidence for the older code scope they tested, but must not be silently promoted to a newer modified production scope.

## Current active direction

1. verify current Arduino Uno Build;
2. perform targeted current-head ESP32<->Arduino UART/hardware smoke when hardware is available;
3. fix only concrete current failures;
4. use measured data for performance/storage changes;
5. otherwise continue with the user's current requested product work.

Do **not** restart old archive/pagination/backup/KG_FIRST/JOB-cancel tasks merely because historical checkpoints contain old `next` sections.

Current authoritative recovery checkpoint:

```text
docs/PROJECT_HANDOFF/61_CURRENT_RECOVERY_AND_DOCS_BASELINE_2026-08-21.md
```
