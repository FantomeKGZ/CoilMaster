# CoilMaster v1 — release ready

Дата: **2026-08-16**  
Репозиторий: `FantomeKGZ/CoilMaster`  
Source of truth: `cmp-protocol-v1`

## Финальный статус

Пользователь подтвердил после полного release-candidate цикла: **«все работает отлично»**.

Результат: **CoilMaster v1 = RELEASE READY**.

Текущая оценка готовности: **100%** в рамках согласованного production scope v1.

Это означает, что обязательные production-функции, safety boundaries, recovery/backup paths и hardware acceptance gates завершены без известных release-blocking замечаний.

## Подтверждённые production gates

На реальном устройстве подтверждены:

- Arduino local winding hardware path;
- ESP32 ↔ Arduino UART exchange;
- linked repair → motor → exact ACTIVE spool flow;
- immutable job snapshot + spool selection;
- JOB_ACK ACCEPTED;
- physical START;
- RUN_STARTED / RUN_COMPLETED;
- manual exact-run wire writeoff;
- costing/finalization/CLOSED;
- stable/readable backup;
- backup-while-active negative gate;
- positive operator-only transactional restore apply;
- reboot without restore/apply auto-resume;
- persisted restore evidence fail-closed cleanup boundary;
- motor import through production UI/API with persistence after reboot;
- read-only microSD capacity diagnostics;
- final populated-device reboot/data/recovery/backup/network/time/diagnostics acceptance;
- stable Wi-Fi operation from the corrected external power supply.

## Production baseline

Hardware-accepted ESP32/Web production baseline remains:

```text
cfcf2b7fb2f7f3376a97179f28303b0e9e0e295a
Show read-only microSD capacity diagnostics
```

Confirmed ESP32 build:

```text
ESP32 Build run 31938372488 — SUCCESS
```

After that baseline, release closure changed only tests/workflow/handoff documentation; production firmware/web paths were not changed before this release-ready declaration.

## Repo-level release protection

CI guards include:

```text
Tests/Web/check_web_assets.js
Tests/Web/check_release_contracts.js
Tests/Web/check_final_acceptance_contracts.js
```

Confirmed release-candidate CI before final closure:

```text
CMP Protocol Tests run 31941111206 — SUCCESS
head: df188ca49d95ee4953bd228c05aec849dcd947b5
```

The run completed protocol tests, web/navigation audits, release safety contracts and final acceptance contracts successfully.

## Safety invariants remain part of the release contract

- physical START only from physical operator input;
- ESP32/Web never control SSR directly;
- no auto-resume after reboot;
- `RUN_COMPLETED` never performs automatic wire writeoff;
- wire writeoff remains manual and exact `spool_id + source_session_id + source_run_id`;
- backup restore remains operator-only, transactional and fail-closed;
- reboot never continues restore/apply automatically;
- persisted restore evidence blocks new backup/restore until explicit cleanup;
- filling microSD never triggers automatic deletion of production data.

Any future change that touches one of these contracts requires targeted re-verification before calling the modified state release-ready.

## Optional hardening — not release blocking

The following are explicitly outside the mandatory v1 release gate and may be performed later without changing the current release-ready status:

- destructive corruption / intentional power-loss / fault injection on a **disposable** microSD or image;
- formal capture of the exact Arduino flashed commit during the next planned Arduino firmware update;
- explicit mDNS convenience check for `http://coil.local/` if desired; IP fallback remains the required operational access path.

Do not perform destructive fault injection on the working production microSD.

## Closure rule

CoilMaster v1 should now be treated as a stable release baseline. Closed hardware gates must not be repeated simply because docs/tests change. Re-run only the tests relevant to production code that actually changes.
