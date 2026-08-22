# Активная работа и следующие шаги

Дата обновления: **2026-08-22**  
Ветка: **`cmp-protocol-v1`**

Этот файл — единственный handoff-файл с активной очередью. Старые checkpoints — history/evidence, а не backlog.

## Verification baseline

Последний явно подтверждённый пользователем GREEN state:

```text
51ea46c1823a451e7f80ecd188daf896aafc752d
Fix production conductor cleanup contract
USER CONFIRMED GREEN
```

Run `32561236301` относится к старому SHA `68446e95...` и имел единственную ошибку: production conductor regression-test пытался открыть уже удалённый legacy `CM_ConductorSettings.h`. Это исправлено в `51ea46c1...`; новый run после исправления пользователь подтвердил GREEN.

Любые commits после `51ea46c1...` требуют нового подтверждения или exact workflow result. Старый красный run не является текущей регрессией.

## Full-code audit status

Repo-review/source-contract этапы завершены:

```text
A Arduino safety/realtime/UART/resources                  COMPLETE
B ESP32 runtime/API/persistence/integrity/network/backup COMPLETE
C desktop/mobile/shared Web parity/error/security        COMPLETE
D tests/CI/build-filter/path-trigger audit               COMPLETE
E docs/AI routing consistency                            COMPLETE
```

Authoritative detail: `docs/PROJECT_HANDOFF/63_FULL_CODE_AUDIT_2026-08-22.md`.

## Закрытые findings — не возвращать без concrete regression

```text
A-001..A-007 JOB/UART admission/parser/correlation/recovery/ordering
B-001 backup runtime Unavailable -> fail closed
B-002 global restore/apply production-mutation interlock
B-003 network API HTTP/storage semantics
B-004 recoverable JobStateStore atomic replacement
B-005 provenance-safe linked JOB preparation transaction
B-006 committed-first NetworkProfileStore recovery
B-007 committed-first RemoteBackupSettingsStore recovery
B-009 production /data/settings/conductor.json atomic transaction
B-010 verified warehouse spool swap / rollback
B-011 verified material ledger swap / rollback
C-001 strict network JSON string escaping
```

Legacy B-008 `CM_ConductorSettingsStore` implementation has now been removed during cleanup; production settings remain owned by `WarehouseStore` through `/data/settings/conductor.json`.

## Current active phase — controlled cleanup

Full audit is complete. Active work is repository cleanup/de-duplication with dependency proof before deletion:

```text
DELETE  proven unused and unreferenced by production/build/tests/docs/runtime
MERGE   duplicate implementation; retain one authoritative owner
KEEP    active production/build/test/docs/history/operator dependency
REVIEW  uncertain dependency; do not delete
```

### Cleanup completed after audit

Removed:

```text
firmware/esp32/src/CM_ConductorSettings.cpp
firmware/esp32/src/CM_ConductorSettings.h
Tests/Web/check_conductor_settings_atomic_recovery.js
its obsolete CMP Protocol Tests workflow step
.github/workflows/README.md
.github/ISSUE_TEMPLATE/README.md
LICENSE                              empty placeholder
CONTINUE_CMP_PROTOCOL_V1.md          stale checkpoint-61 continuation entrypoint
```

Production conductor regression-test was updated after the legacy source deletion so it checks the authoritative owner and asserts that legacy source files do not return.

Root `README.md` and `AGENTS.md` have been rerouted from stale audit/checkpoint instructions into the cleanup phase.

### Current cleanup candidates

```text
firmware/esp32/web/shared/calculator-multisource.js
  proven runtime-dead with the new single-line sourceWires UI,
  but CM_StaticSiteServer still injects it; remove injection and asset together.

Docs/
  capitalized legacy foundation documentation; classify file-by-file before deletion.

BUILD_INFO.md
  useful build reference but partially overlaps current handoff; REVIEW.

PROJECT.manifest
  current source/boundary manifest; KEEP.
```

Do not delete by filename resemblance alone. Capitalized `Docs/`, `Shared/Protocol/`, `Arduino/`, `Core/` and other older-looking directories may still be build/test/history dependencies.

## Calculator — current production contract

```text
source input: one line, up to 5 wires, e.g. 0,51;0,71;0,95
warehouse recommendations: based on current warehouse catalogue
standard recommendations: read-only IEC 60317 R20 project catalogue
```

Current diameter model is `diameterHundredthsMm` (0.01 mm precision). The IEC R20 reference is adapted to this precision; moving to 0.001 mm requires an explicit data/model migration.

## Persistence resilience item that is NOT cleanup

Direct append tail resilience for new spool/material catalogue records remains a non-blocking P2 design item. Do not auto-truncate malformed/torn production NDJSON and do not treat it as cleanup; automatic deletion of production evidence is forbidden.

## External hardware verification gate

Targeted ESP32<->Arduino smoke remains required when the stand is available but does not block cleanup of proven-unused software assets:

```text
normal JOB -> Arduino READY
physical START only
RUN_STARTED -> RUN_COMPLETED
repeat > 1 -> physical START each run
no automatic material writeoff
zero-run cancel / ALREADY_CLEAR / safe ALL_CLEAR
late zero-id ALL_CLEAR must not cancel fresh job
lost JOB_ACK -> timeout/manual review -> late RUN_STARTED reconciliation
reboot waiting/running -> no auto resume
```

Hardware GREEN is not inferred from CI.

## Safety boundary

Never change during cleanup:

- physical START only physical/local;
- no automatic START between repeats;
- no auto-resume after reboot;
- Arduino owns SSR;
- ESP32/Web do not drive SSR directly;
- timeout/lost ACK alone never proves Arduino idle;
- final repeat cannot reopen automatically;
- RUN_COMPLETED never auto-writes off material;
- manual writeoff uses exact source_session_id + source_run_id;
- KG_FIRST spool omission only approved unallocated/manual path;
- exact spool provenance when spool is used;
- backup/restore explicit, operator-only, transactional/fail-closed;
- no automatic production-data deletion.

## Next cleanup sequence

1. remove dead `calculator-multisource.js` injection + asset as one dependency-closed change;
2. classify `Docs/` against current lowercase `docs/` and references; delete only proven superseded files;
3. audit root transition files (`BUILD_INFO.md`, `REGISTER.md`, `CHANGELOG.md`, `ARCHITECTURE.md`) for duplication/current references;
4. scan remaining source/test files for owners compiled but never instantiated or tests no longer wired to CI;
5. after each meaningful deletion batch, run applicable CI; do not continue a broad destructive batch through a new red result.

## Read order

```text
/AGENTS.md
docs/PROJECT_HANDOFF/00_READ_FIRST.md
docs/PROJECT_HANDOFF/63_FULL_CODE_AUDIT_2026-08-22.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
docs/PROJECT_HANDOFF/65_UART_DESYNC_AND_TIMEOUT_RECOVERY_AUDIT_2026-08-22.md
docs/PROJECT_HANDOFF/66_ESP32_BUILD_ID_CI_RECOVERY_2026-08-22.md
docs/AI_AGENT/00_START_HERE.md
docs/AI_AGENT/02_CHANGE_ROUTER.md
docs/AI_AGENT/04_VERIFICATION_MATRIX.md
```
