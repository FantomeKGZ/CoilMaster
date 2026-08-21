# CoilMaster — verification matrix for AI agents

This document answers: **what must be verified after a specific class of change.**

Do not claim a workflow is green until its actual run has completed successfully. A Git commit is not a build result.

## 1. Current known automated baseline

On production commit:

```text
e35c4bfe0cef3c2342ad6b27e43cc931fe14dd00
```

verified:

```text
CMP Protocol Tests #2175 — GREEN
ESP32 Build #1241 — GREEN
```

This does not imply a current-head Arduino Uno Build or hardware acceptance result.

## 2. Available automated gates

### Arduino Uno Build

```text
.github/workflows/arduino-uno-build.yml
pio run -e uno
```

Production sources selected by `platformio.ini`:

```text
Core/*.cpp
Arduino/*.cpp
firmware/arduino/src/main.cpp
```

### ESP32 Build

```text
.github/workflows/esp32-build.yml
pio run -e esp32
```

Production source filter:

```text
firmware/esp32/src/*.cpp
```

### CMP Protocol Tests + Web/release audits

Workflow:

```text
.github/workflows/cmp-protocol-tests.yml
```

It includes host protocol/state-machine tests plus the configured `Tests/Web/*.js` contract audits. The workflow is intentionally configured so later audits still run after an earlier audit failure; any failed audit still fails the job.

## 3. Verification matrix

| Change type | Arduino Build | ESP32 Build | CMP/Web audits | Hardware regression |
|---|---:|---:|---:|---:|
| Docs only | No | No | Usually no | No |
| `Tests/*` only | No unless source coupled | No unless source coupled | Yes for affected tests | No |
| Desktop/mobile/shared web only | No | Workflow may run because it watches `firmware/esp32/**` | Yes | Only if device behavior needs proof |
| ESP32 C++ service/API/storage | No | **Yes** | Usually **Yes** | Targeted when runtime/persistence/hardware behavior changed |
| Arduino `Core/` or `Arduino/` | **Yes** | No unless protocol peer changed | Usually **Yes** | Targeted for machine/input/output behavior |
| UART/CMP1 wire contract | **Yes** | **Yes** | **Yes** | **Yes**, targeted cross-board regression |
| Physical START / SSR / Hall / machine state | **Yes** | If peer/service changed | Relevant audits | **Mandatory targeted hardware test** |
| Workshop persisted schema | No | **Yes** | **Yes** | Persistence/reboot when semantics changed |
| Warehouse/material writeoff | No | **Yes** | **Yes** | Targeted exact-run/writeoff test when production logic changed |
| Backup/restore/apply | No | **Yes** | **Yes** | Targeted safe restore/reboot gate |
| Network/FTP | No | **Yes** | Relevant Web contracts | Targeted device/network test |
| Build/workflow config | Affected build | Affected build | Affected workflow | No unless binary/behavior changes |

## 4. Hardware gates are change-scoped

Do not repeat a historical full E2E suite for docs/test-only changes or unrelated static contract hardening.

Do re-run a closed hardware gate when the production code that established that gate changes.

Examples:

- changing Arduino START/SSR state logic -> recheck physical START/safe SSR;
- changing CMP1 framing/cancel/ACK behavior -> recheck ESP32<->Arduino communication;
- changing restore apply -> recheck the relevant restore safety gate;
- changing persisted writeoff semantics -> recheck exact-run manual writeoff behavior.

## 5. Safety-contract verification

Protect these invariants:

```text
physical START only
no automatic START between repeats
Arduino owns SSR
no automatic resume after reboot
RUN_COMPLETED does not auto-writeoff
manual writeoff requires exact source_session_id + source_run_id
spool_id optional only for approved KG_FIRST unallocated/manual path
exact spool provenance preserved whenever a spool is used
operator-only transactional restore
persisted stale restore evidence blocks operations
no automatic production-data cleanup
```

Primary automated guards include:

```text
Tests/Web/check_release_contracts.js
Tests/Web/check_final_acceptance_contracts.js
Tests/Web/check_kg_first_material_contracts.js
Tests/Web/check_writeoff_fault_contracts.js
Tests/Web/check_hall_calibration_contracts.js
```

Use the exact current workflow/script names from `.github/workflows/cmp-protocol-tests.yml` if they change.

## 6. JOB cancel/recovery verification

Current repo implementation is closed unless a regression is observed.

If code touching this boundary changes, verify at least:

```text
no-run remote cancel succeeds
already-clear is idempotent
active physical run cannot be cleared
D -> * -> # -> D fallback emits ALL_CLEAR only when safe
ALL_CLEAR never means RUN_COMPLETED
reboot does not auto-start or auto-complete
```

A docs-only or unrelated change must not reopen this hardware block automatically.

## 7. Protocol verification

Any change to `CMP1|...` must cover valid frame, bad CRC, field count, numeric range, unknown type/status, stale/duplicate identity where relevant, timeout/retry behavior and staged peer compatibility.

Run:

```text
CMP Protocol Tests
Arduino Uno Build
ESP32 Build
```

Then perform targeted two-board hardware verification if wire behavior changed.

## 8. Persistence verification

For changes that create/modify `/data` records or files, verify applicable cases:

```text
valid read/write
reboot persistence
malformed row/file fails closed
old record compatibility
interrupted/pending recovery
identity/reference consistency
integrity audit coverage
backup inclusion
restore-plan/rollback/apply coverage
```

## 9. UI verification

Check both:

```text
firmware/esp32/web/desktop/
firmware/esp32/web/mobile/
```

UI must not hide server errors, invent authoritative totals, imply physical motion from queued state, silently downgrade corruption, or offer automatic destructive cleanup/restore.

## 10. Backup/restore verification levels

- **Repo-level only** for docs/tests/refactors without runtime behavior change.
- **Device non-destructive** for preflight/inspection/safe-idle logic changes.
- **Destructive fault injection** only on disposable media/filesystem images, never the working production microSD.

## 11. Result labels

```text
NOT VERIFIED       result unavailable/not run
FAILED             named gate ran and failed
SUCCESS / GREEN    named workflow/test completed successfully
USER CONFIRMED     user explicitly verified real-device behavior
APPROVED           architecture/contract decision accepted
```

## 12. Current-baseline rule

Current authoritative project status is selected by:

```text
docs/PROJECT_HANDOFF/00_READ_FIRST.md
docs/PROJECT_HANDOFF/61_CURRENT_RECOVERY_AND_DOCS_BASELINE_2026-08-21.md
```

Older release checkpoint 38 and earlier numbered checkpoints are historical evidence, not the current active-work baseline.

If production firmware/web behavior changes, the affected scope becomes a new candidate until relevant automated and hardware gates pass.

## 13. Before saying "done"

```text
[ ] current target files fetched before edit
[ ] current blob SHA used
[ ] source branch is cmp-protocol-v1
[ ] ownership/lifecycle remains explicit
[ ] safety invariants preserved
[ ] persisted-data implications handled
[ ] desktop/mobile parity handled if relevant
[ ] applicable automated gate actually passed or is explicitly NOT VERIFIED
[ ] hardware regression passed if required
[ ] AI docs updated if topology/contract location changed
[ ] old historical checkpoint was not accidentally treated as active work
```
