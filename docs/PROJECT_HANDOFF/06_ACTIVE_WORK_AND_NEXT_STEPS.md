# Активная работа и следующие шаги

Дата обновления: **2026-08-22**  
Ветка: **`cmp-protocol-v1`**

Этот файл — единственный handoff-файл с активной очередью. Старые checkpoints — history/evidence, а не backlog.

## Verification baseline

Последний явно подтверждённый пользователем GREEN state:

```text
3ebc942f1be9397af9d8ee5336c0ed78e9b13c87
Record calculator standard alternatives feature
USER CONFIRMED GREEN
```

Любые commits после этого SHA требуют нового подтверждения или exact workflow result. Пустой combined-status не считается GREEN.

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
B-008 committed-first legacy ConductorSettingsStore recovery
B-009 production /data/settings/conductor.json atomic transaction
B-010 verified warehouse spool swap / rollback
B-011 verified material ledger swap / rollback
C-001 strict network JSON string escaping
```

Network JSON now escapes quote/backslash, standard JSON control sequences and every remaining `0x00..0x1f` byte. Both `CM_NetworkWeb.cpp` profile/scan responses and `/api/system/network` runtime fields are covered by `Tests/Web/check_network_json_escaping.js`.

## Tests/CI audit changes after last GREEN

Added/connected:

```text
Tests/Web/check_material_ledger_atomic_recovery.js
Tests/Web/check_ci_trigger_contracts.js
Tests/Web/check_network_json_escaping.js
```

Fixed trigger gaps:

```text
Arduino Uno Build watches Shared/**
CMP Protocol Tests PR watches Shared/**
```

This protects the real production dependency on `Shared/CMP1Text/CM_Cmp1Crc.h`.

`motor-reference.yml` remains pinned to `cmp-protocol-v1`; `main` is not an implementation source.

## Current active queue

1. obtain applicable exact-current CI/GREEN for the post-`3ebc942f...` audit package;
2. if GREEN, begin the already approved repository cleanup/de-duplication phase;
3. after cleanup run full applicable CI and compare the final tree against the pre-cleanup baseline.

Current connector endpoints do not expose a completed exact-current workflow result for the latest HEAD, therefore current post-baseline state is **NOT VERIFIED**.

## Post-audit cleanup — approved, but waits for current CI gate

Before deleting anything build a repo-wide dependency inventory and classify each candidate:

```text
DELETE  proven unused and unreferenced by production/build/tests/docs/runtime
MERGE   duplicate implementation; retain one authoritative owner
KEEP    active production/build/test/docs/history/operator dependency
REVIEW  uncertain dependency; do not delete
```

Known candidates to prove:

```text
firmware/esp32/src/CM_ConductorSettings.h/.cpp   legacy parallel persistence
firmware/esp32/web/shared/calculator-multisource.js
.github/workflows/README.md                      one-byte placeholder
.github/ISSUE_TEMPLATE/README.md                 placeholder candidate; recheck exact contents
```

`calculator-multisource.js` is still injected by `CM_StaticSiteServer`, but current calculator pages no longer expose legacy `#diameter/#strands`, so the helper returns immediately. Cleanup must remove both the dead asset and its injection only after dependency proof.

Do not auto-delete malformed/torn production NDJSON. Direct append tail resilience for new spool/material catalogue records remains a non-blocking P2 design item; automatic truncation of production evidence is forbidden.

## Калькулятор — current production contract

```text
source input: one line, up to 5 wires, e.g. 0,51;0,71;0,95
warehouse recommendations: based on current warehouse catalogue
standard recommendations: read-only IEC 60317 R20 project catalogue
```

Current diameter model is `diameterHundredthsMm` (0.01 mm precision). The IEC R20 reference is adapted to this precision; moving to 0.001 mm requires an explicit data/model migration.

## External hardware verification gate

Targeted ESP32<->Arduino smoke remains required when the stand is available but does not block repo cleanup of proven-unused software assets:

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
