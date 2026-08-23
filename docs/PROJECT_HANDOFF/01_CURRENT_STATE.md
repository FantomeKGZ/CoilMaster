# Текущее состояние CoilMaster

Дата обновления: **2026-08-23**  
Ветка: **`cmp-protocol-v1`**

Этот файл описывает только **текущее** состояние. Исторические детали находятся в numbered checkpoints и не являются очередью активных работ.

## Source of truth

Единственный источник реализации — `cmp-protocol-v1`. `main` не использовать как источник кода.

Перед изменением existing file fetch текущего содержимого и blob SHA. Не объявлять build/CI/hardware success без фактического результата.

## Current verified repo baseline — checkpoint 62

Production ESP32 C++ hardening завершён на:

```text
5fa6bcea812c33f0b2dc8e13baae476221839b3a
Validate session state against repeat target
```

Verified Actions:

```text
ESP32 Build #1245 — GREEN
run 32515224487
head_sha 5fa6bcea812c33f0b2dc8e13baae476221839b3a

CMP Protocol Tests #2210 — GREEN
run 32515361340
head_sha ba3ac4bb69a038a0d7ea2d2dabedbd5f63569133
```

После `5fa6bcea` до protocol run `ba3ac4bb` менялись только regression tests и документация, не ESP32 production C++.

Checkpoint 62 repo-level automated verification закрыт.

## Checkpoint 62 lifecycle/integrity hardening

```text
ce38711c  Keep timed-out jobs in manual review
2ecebb5e  Ignore stale cancel after completed job
9e68bd86  Reject runs beyond immutable repeat target
5fa6bcea  Validate session state against repeat target
```

Regression coverage:

```text
Tests/Web/check_job_cancel_recovery_contracts.js
fa5a0073 / 44d1e037 / 5188084d / 5e407f2e
```

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

Production cross-board protocol: text `CMP1|...`. `Shared/Protocol/` is older binary host-test code, not production wire protocol.

## Safety boundary

- physical START только local/physical;
- no automatic START between repeats;
- no auto-resume after reboot;
- ESP32/Web never drive SSR directly;
- lost ACK / `TIMED_OUT` never proves Arduino idle;
- final repeat cannot reopen automatically;
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

## JOB lifecycle / recovery

Implemented and repo-level verified:

- no-run remote JOB cancel;
- lost-ACK cancel escalates to remote `JOB_CANCEL` rather than local-only closure;
- Arduino idempotent `ALREADY_CLEAR` when remote job is absent;
- physical fallback `D -> * -> # -> D`;
- fallback emits `ALL_CLEAR`, never `RUN_COMPLETED`;
- active physical run prevents unsafe clear;
- positive remote cancel closes persisted state only with zero physical-run evidence;
- `TIMED_OUT` requires manual review and cannot use ordinary inactive dismiss;
- stale cancel after `ProgramCompleted`/`ClosedAfterReview` is a persisted-state no-op;
- immutable `repeatTarget` is enforced before winding journal append;
- `RUN_STARTED` cannot reopen a completed target;
- `RUN_COMPLETED` above immutable target is rejected before NDJSON mutation;
- deep session audit checks persisted completed runs against immutable target;
- recovery is re-evaluated before a new job may be created;
- reboot never auto-starts and never fabricates completion/writeoff;
- immutable snapshots/history are not erased by operational cancellation.

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

Backup deep validation is read-only and fail-closed. Session persistence preflight is owned by `WindingSessionPersistenceIntegrityAudit`; duplicate full session-directory preflight in backup export was removed. Checkpoint 62 additionally validates snapshot/state repeat-target consistency without another full journal pass.

## UI/API state

Desktop and mobile interfaces cover the implemented workshop, motor/import, repairs, linked winding, Arduino archive, materials/warehouse, costing, reports, settings, backup/network and diagnostics flows. Growing collections use bounded/paged APIs where implemented; old checkpoints describing their migration are historical, not active work.

### Winding reference product integration — repo-level GREEN

The legacy static winding reference in `FantomeKGZ/motor-winding-reference/sourse/{desktop,mobile}` is being integrated as a CoilMaster section without changing production runtime ownership.

Current implementation includes:

- common desktop/mobile CoilMaster-style reference shell;
- full navigation from reference pages back to CoilMaster workshop sections;
- phone-accessible horizontal section navigation;
- desktop/mobile switch via the existing `cm-ui-version` preference;
- shared reference CSS/JS;
- dedicated legacy importer and integrity checker;
- legacy Windows-1251 -> UTF-8 conversion;
- removal of the old `div.verh` / `images/verh.jpg` top banner only;
- content/table/description/internal-link preservation contract;
- global SHA-256 deduplication of repeated non-CSS assets with MIME-safe suffix separation;
- mode-specific UTF-8 CSS with rewritten local URLs, imports and embedded styles;
- report-only migration audit without automatic deletion;
- full-page content fidelity gate for normalized visible text and table/link/image structure;
- CI dry-build workflow for source checkout, import, integrity check and footprint reporting.

Generated legacy content is verified and published as a bounded GitHub Actions artifact without committing 1852 generated legacy pages to Git.

Verified reference run:

```text
Reference Legacy Import Check run 32641366079 — GREEN
head_sha cea85a09dd4208bf88545284ae80a12edce22681
```

Measured bundle:

```text
926 desktop + 926 mobile generated pages
926 shared catalog entries
329M reference / 331M full web bundle
4626 reference files / 4693 full web files
2769 shared assets / 0 desktop-mobile duplicate unique assets
artifact coilmaster-web-sd-bundle-cea85a09dd4208bf88545284ae80a12edce22681
```

Generated pages include top/bottom return-to-search navigation, responsive images and horizontally scrollable wide tables. The operator completed the physical microSD/ESP32 web smoke on 2026-08-23 and reported no errors for the verified bundle.

### Reference site readiness

Current estimate:

```text
content migration:                       100%
migration automation/integrity:          99%
UX and offline SD bundle:                98%
overall software/site readiness now:     about 98%
```

Commits `e89813f2...` and `fe2aeed1...` add complete source/generated visible-text and structural-count comparison for every page. The operator confirmed the full 926 + 926 page Actions batch GREEN.

Manifest schema v2, full content fidelity, complete table/image UX contracts and full `/web` offline dependency closure are operator-confirmed GREEN. The workflow now publishes a risk-based physical smoke plan. Remaining reference-site release gate is physical microSD/ESP32 smoke of the fresh bundle using that plan. The separate two-board UART hardware checkpoint is not included in this reference-site percentage.

## Verification still separate

Repo-level checkpoint 62 is GREEN. Hardware remains a separate gate:

```text
two-board UART hardware smoke checkpoint 62 — NOT VERIFIED
full hardware acceptance — only when affected scope requires it
```

Reference Legacy Import Check run `32641366079` is GREEN, the operator confirmed all current Actions GREEN after the CMP regex correction, and the physical microSD/ESP32 reference smoke completed without errors. The separate two-board UART/repeat/cancel hardware gate remains NOT VERIFIED.

## Current active direction

1. verify and deploy the refreshed web bundle containing the shared-shell `📚 Справочник` entry, then smoke that entry on desktop/mobile;
2. perform targeted ESP32<->Arduino UART/repeat/cancel smoke when hardware is available;
3. fix only concrete current failures;
4. use measured data for performance/storage changes.

Do **not** restart old archive/pagination/backup/KG_FIRST/JOB-cancel implementation work merely because historical checkpoints contain old `next` sections.

Current active queue:

```text
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
```

Current authoritative checkpoint:

```text
docs/PROJECT_HANDOFF/62_JOB_LIFECYCLE_SAFETY_HARDENING_2026-08-22.md
```
