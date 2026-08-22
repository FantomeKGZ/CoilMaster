# CoilMaster — verification matrix for AI agents

This document answers: **what must be verified after a specific class of change.**

Do not claim a workflow is green until its actual run has completed successfully. A Git commit is not a build result.

## 1. Current known automated baseline

Latest operator-confirmed branch baseline at the time of this update:

```text
3ebc942f1be9397af9d8ee5336c0ed78e9b13c87
Record calculator standard alternatives feature
USER CONFIRMED GREEN
```

Commits after that SHA require a later explicit operator confirmation or exact matching workflow result. Older named workflow runs remain historical evidence only.

Current active project status is selected by checkpoint 63 plus active queue 06, not an older recovery checkpoint.

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

The Arduino transport also consumes `Shared/CMP1Text/CM_Cmp1Crc.h`, so `Shared/**` must trigger this build.

### ESP32 Build

```text
.github/workflows/esp32-build.yml
pio run -e esp32
```

Production source filter:

```text
firmware/esp32/src/*.cpp
```

`Shared/**`, `platformio.ini` and `scripts/platformio_build_id.py` are also compile-relevant and must trigger the ESP32 build.

### CMP Protocol Tests + Web/release audits

Workflow:

```text
.github/workflows/cmp-protocol-tests.yml
```

It includes host protocol/state-machine tests plus configured `Tests/Web/*.js` contract audits. The workflow intentionally lets later audits run with `if: always()` after an earlier failure so one run exposes multiple failures; any failed step still fails the job.

`Tests/Web/check_ci_trigger_contracts.js` protects critical workflow path coverage.

## 3. Verification matrix

| Change type | Arduino Build | ESP32 Build | CMP/Web audits | Hardware regression |
|---|---:|---:|---:|---:|
| Docs only | No | No | Usually no | No |
| `Tests/*` only | No unless source coupled | No unless source coupled | Yes for affected tests | No |
| Desktop/mobile/shared web only | No | Workflow may run because it watches `firmware/esp32/**` | **Yes** | Only if device behavior needs proof |
| ESP32 C++ service/API/storage | No | **Yes** | Usually **Yes** | Targeted when runtime/persistence/hardware behavior changed |
| Arduino `Core/` or `Arduino/` | **Yes** | No unless protocol peer changed | Usually **Yes** | Targeted for machine/input/output behavior |
| `Shared/**` production protocol/CRC | **Yes** | **Yes** when ESP32 consumes it | **Yes** | Targeted if wire behavior changed |
| UART/CMP1 wire contract | **Yes** | **Yes** | **Yes** | **Yes**, targeted cross-board regression |
| Physical START / SSR / Hall / machine state | **Yes** | If peer/service changed | Relevant audits | **Mandatory targeted hardware test** |
| Workshop persisted schema | No | **Yes** | **Yes** | Persistence/reboot when semantics changed |
| Warehouse/material writeoff | No | **Yes** | **Yes** | Targeted exact-run/writeoff test when production logic changed |
| Backup/restore/apply | No | **Yes** | **Yes** | Targeted safe restore/reboot gate |
| Network/FTP | No | **Yes** | Relevant Web contracts | Targeted device/network test |
| Build/workflow config | Affected build | Affected build | Affected workflow | No unless binary/behavior changes |
| Post-audit cleanup/deletion | According to affected dependency graph | According to affected dependency graph | **Yes** for affected contracts | Only when production/hardware behavior changed |

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
Tests/Web/check_job_cancel_recovery_contracts.js
Tests/Web/check_restore_mutation_interlock.js
Tests/Web/check_job_state_atomic_replace.js
Tests/Web/check_job_preparation_transaction.js
Tests/Web/check_warehouse_spool_atomic_recovery.js
Tests/Web/check_material_ledger_atomic_recovery.js
Tests/Web/check_final_acceptance_contracts.js
Tests/Web/check_kg_first_material_contracts.js
Tests/Web/check_writeoff_fault_contracts.js
Tests/Web/check_hall_calibration_contracts.js
Tests/Web/check_ci_trigger_contracts.js
```

Use the exact current workflow/script names from `.github/workflows/cmp-protocol-tests.yml` if they change.

## 6. JOB cancel/recovery verification

Current repo implementation is closed unless a regression is observed.

The dedicated static guard is:

```text
Tests/Web/check_job_cancel_recovery_contracts.js
```

It protects transport and persisted recovery semantics, including lost-ACK timeout reconciliation and zero-id ALL_CLEAR isolation. A docs-only or unrelated change must not reopen this hardware block automatically.

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

For atomic replacement, rename alone is not commit proof. Validate prepared temp before promotion, validate authoritative main after promotion, and keep/restore the last valid backup until the new main is proven readable and structurally valid.

Do not automatically truncate/delete corrupted production evidence as a convenience recovery mechanism.

## 9. UI verification

Check both:

```text
firmware/esp32/web/desktop/
firmware/esp32/web/mobile/
firmware/esp32/web/shared/
```

UI must not hide server errors, invent authoritative totals, imply physical motion from queued state, silently downgrade corruption, or offer automatic destructive cleanup/restore.

For dynamic HTML, verify operator/server-controlled strings are escaped or inserted through `textContent`. For JSON assembled manually in C++, verify complete JSON string escaping, including control bytes.

Current calculator contract includes one semicolon-separated source-wire field (1..5 wires), separate warehouse recommendations and read-only standard-reference alternatives.

## 10. Backup/restore verification levels

- **Repo-level only** for docs/tests/refactors without runtime behavior change.
- **Device non-destructive** for preflight/inspection/safe-idle logic changes.
- **Destructive fault injection** only on disposable media/filesystem images, never the working production microSD.

## 11. Result labels

```text
NOT VERIFIED       result unavailable/not run
FAILED             named gate ran and failed
SUCCESS / GREEN    named workflow/test completed successfully
USER CONFIRMED     user explicitly confirmed visible workflow/hardware state
APPROVED           architecture/contract decision accepted
```

## 12. Current-baseline rule

Current authoritative project status is selected by:

```text
docs/PROJECT_HANDOFF/00_READ_FIRST.md
docs/PROJECT_HANDOFF/63_FULL_CODE_AUDIT_2026-08-22.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
```

Older numbered checkpoints are historical evidence, not the current active-work baseline.

If production firmware/web behavior changes, the affected scope becomes a new candidate until relevant automated and hardware gates pass.

## 13. Post-audit cleanup verification

Cleanup starts only after the full audit and final cross-layer recheck.

Before deleting/merging a candidate, prove:

```text
C++ include/call-site ownership
PlatformIO build_src_filter impact
workflow and test references
static web/script injection and imports
runtime microSD/data-path compatibility
migration/history requirements
AI/docs routing references
```

Classify each candidate DELETE/MERGE/KEEP/REVIEW. After cleanup, run every applicable gate for the files actually removed/merged and compare the resulting tree against the pre-cleanup inventory.

## 14. Before saying "done"

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
[ ] cleanup deletion was dependency-proven if this was cleanup work
```
