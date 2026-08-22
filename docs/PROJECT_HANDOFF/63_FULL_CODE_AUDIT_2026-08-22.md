# Checkpoint 63 — Full code audit phase

Date: 2026-08-22  
Branch: `cmp-protocol-v1`

## Purpose

This checkpoint owns the full-code audit. Phase 9 Web implementation is complete in checkpoint 64. Targeted JOB/UART review is summarized in checkpoint 65. ESP32 build-identity CI recovery is recorded in checkpoint 66.

Historical checkpoints are evidence, not an automatic task queue.

## Verification baseline

Last operator-confirmed GREEN state:

```text
3ebc942f1be9397af9d8ee5336c0ed78e9b13c87
Record calculator standard alternatives feature
USER CONFIRMED GREEN
```

Commits after this SHA are **NOT VERIFIED** until an applicable exact-current workflow result or explicit operator confirmation is available. Empty GitHub combined-status output is not proof of GREEN.

## Audit scope and status

```text
A. Arduino safety/realtime/UART/resources                  COMPLETE
B. ESP32 runtime/API/persistence/integrity/network/backup COMPLETE
C. Desktop/mobile/shared Web parity/error/security        COMPLETE
D. Tests/CI/build filters/triggers                         COMPLETE
E. Documentation/AI routing consistency                   COMPLETE
F. Final applicable CI gate                               PENDING
G. Post-audit repository cleanup                          APPROVED / waits for F
```

## Severity

```text
P0 immediate physical/data safety risk or destructive corruption path
P1 serious functional/state/persistence defect likely to affect production
P2 concrete robustness/performance/maintainability weakness
P3 low-risk cleanup/dead code/docs/test-quality issue
```

## Findings closed in this audit

### A-001..A-007 — FIXED

Remote JOB admission/parser/correlation, zero-id ALL_CLEAR recovery identity, stale cancel handling, control/RUN ordering and late RUN after lost JOB_ACK were hardened. No physical START or SSR authority moved to ESP32/Web.

### B-001 — P1 — backup activity guard unknown runtime — FIXED

Runtime must be proven `Safe`; `Busy` and `Unavailable` remain fail-closed.

### B-002 — P1 — restore/apply production mutation race — FIXED

One process-wide restore mutation interlock blocks non-GET `/api/*` while APPLY/rollback is active. Forward apply rechecks live activity and enters rollback if Safe can no longer be proven. GET/status remain available. Operator-only apply/no-auto-resume remain unchanged.

### B-003 — P2 — network API HTTP/storage semantics — FIXED

400 validation, 404 absent profile, 409 conflict/capacity, 500 persistence failure and 503 unavailable store/manager are separated.

### B-004 — P1 — JobStateStore destructive replacement boundary — FIXED

Authoritative replacement is temp-write/verify -> main to `.bak` -> temp to main -> verify committed main -> delete `.bak`. Interrupted evidence fails closed.

### B-005 — P2 — linked JOB preparation partial transaction — FIXED

```text
snapshot
-> CREATED + WAITING_DELIVERY + zero-run state
-> exact spool selection when linked
-> DELIVERING state
-> UART queueJob
```

No automatic queue/resume was introduced.

### B-006 — P2 — NetworkProfileStore recovery ambiguity — FIXED

Committed main wins; valid backup restores committed state; temp is promoted only for an interrupted first write with no backup.

### B-007 — P2 — RemoteBackupSettings recovery ambiguity — FIXED

Uncommitted prepared settings cannot become authoritative after reboot.

### B-008 — P2 — legacy ConductorSettings recovery ambiguity — FIXED

Legacy store also follows committed-first fail-closed recovery.

### B-009 — P2 — production conductor settings atomic transaction — FIXED

Authoritative production calculator settings remain owned by `WarehouseStore` / `CM_ConductorSettingsWeb` at `/data/settings/conductor.json`; write/recovery path is atomic and regression-protected. Legacy `CM_ConductorSettings.*` is not the production owner.

### B-010 — P1/P2 — warehouse spool swap commit proof — FIXED

Prepared temp and committed main are parser-validated. `.bak` is deleted only after promoted main validates; malformed promoted main rolls back to last valid backup.

### B-011 — P1/P2 — material ledger swap commit proof — FIXED

Material ledger now has the same verified commit/rollback boundary. Dedicated regression: `Tests/Web/check_material_ledger_atomic_recovery.js`.

### C-001 — P2 — network JSON string escaping — FIXED

`CM_NetworkWeb.cpp` and `/api/system/network` now escape quotes, backslashes, standard JSON control characters and remaining `0x00..0x1f` bytes. Runtime network status applies escaping to state/result, AP/STA SSID and hostname fields. Regression: `Tests/Web/check_network_json_escaping.js`.

## Reviewed without a new blocking defect

- `PersistentIdAllocator` temp/high-water/backup recovery.
- immutable `JobSnapshotStore` and `JobSpoolSelectionStore` create paths.
- `RepairRegistry` append-only fail-closed malformed-tail behavior.
- `NetworkManager` AP recovery sequencing.
- `RtcClock` safe-idle NTP write + verify.
- `WebRecoveryFtpServer` `/web`-only scope and activity gating.
- shared app shell, Wi-Fi UI, Arduino archive, winding-history spool metadata, pricing-history and writeoff HTML rendering: inspected dynamic values are escaped or use `textContent`.
- `motor-reference.yml`: checkout/build/push pinned to `cmp-protocol-v1`; `main` is not an implementation source.

## Residual non-blocking findings / cleanup candidates

### Append-only catalogue tail resilience — P2

`WarehouseStore::addSpool()` and `MaterialLedger::addMaterial()` append directly to authoritative NDJSON. A torn tail correctly fails closed, but can make the catalogue unavailable until operator recovery. Do **not** auto-truncate production evidence. Any future improvement must be an explicit transactional append design.

### Repository cleanup candidates — P3 / REVIEW until dependency proof

```text
firmware/esp32/src/CM_ConductorSettings.h/.cpp
firmware/esp32/web/shared/calculator-multisource.js
.github/workflows/README.md
.github/ISSUE_TEMPLATE/README.md
```

The calculator helper is still injected by `CM_StaticSiteServer`, but the current calculator no longer exposes the legacy `#diameter/#strands` controls, so the helper returns immediately. Remove only in cleanup after reference/dependency proof.

## Tests/CI audit results

Added/strengthened:

```text
Tests/Web/check_material_ledger_atomic_recovery.js
Tests/Web/check_ci_trigger_contracts.js
Tests/Web/check_network_json_escaping.js
```

Trigger gaps fixed:

```text
Arduino Uno Build: Shared/** changes trigger UNO compile
CMP Protocol Tests PR: Shared/** changes trigger contracts
```

This protects the real dependency from Arduino transport to `Shared/CMP1Text/CM_Cmp1Crc.h`.

## Docs/AI routing audit results

Authoritative AI docs now route to checkpoint 63 + `06_ACTIVE_WORK_AND_NEXT_STEPS.md`, not checkpoint 61/62 as active work. Current conductor calculator settings ownership is documented as `WarehouseStore` + `CM_ConductorSettingsWeb`; legacy parallel persistence must not become authoritative again.

## Final gate before cleanup

Applicable automated verification is required for the current post-`3ebc942f...` code set. At checkpoint update time current HEAD status from the connector is not available as a completed GREEN result, therefore the state is **NOT VERIFIED**.

After GREEN/explicit confirmation, begin the separately approved cleanup phase with a repo-wide dependency inventory and classify every candidate as:

```text
DELETE proven unused
MERGE duplicate implementation with one authoritative owner
KEEP active production/build/test/docs/history dependency
REVIEW uncertain; do not delete
```

## External hardware verification gate

Still required when the physical stand is available:

```text
normal JOB -> Arduino READY
physical START only
RUN_STARTED -> RUN_COMPLETED
repeat > 1 -> physical START each run
no automatic material writeoff
zero-run cancel / ALREADY_CLEAR / safe physical ALL_CLEAR
late zero-id ALL_CLEAR must not cancel fresh job
lost JOB_ACK -> TIMED_OUT/manual review -> late RUN_STARTED reconciliation
reboot waiting/running -> no auto resume
restore interlock GET remains available; mutations blocked during APPLY
```

Hardware GREEN is never inferred from CI.

## Safety boundary

```text
physical START only
no automatic START between repeats
no auto-resume after reboot
Arduino owns SSR
ESP32/Web never directly drive SSR
RUN_COMPLETED never auto-writes off material
manual writeoff exact source_session_id + source_run_id
spool_id optional only approved KG_FIRST unallocated/manual path
exact spool provenance retained when spool is used
backup restore operator-only, transactional and fail-closed
no automatic production-data cleanup
```
