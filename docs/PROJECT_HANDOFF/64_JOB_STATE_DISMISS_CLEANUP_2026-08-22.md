# Job-state dismiss cleanup correction — 2026-08-22

Ветка: `cmp-protocol-v1`

## Final classification

`firmware/esp32/src/CM_JobStateDismiss.cpp` — **KEEP**.

Метод:

```cpp
JobStateStore::dismissInactive(uint32_t sessionId, uint32_t nowMs)
```

Предыдущая DELETE-классификация была ошибочной и отменена после фактического CI/caller proof.

## Direct dependency proof

`CM_StaticSiteServer.cpp` содержит operator-only endpoint `/api/recovery/acknowledge-and-restart`, который после повторной проверки persisted job identity и immutable snapshot вызывает:

```cpp
states.dismissInactive(sessionId, millis())
```

Для linked job этот endpoint предварительно отказывает с `linked_job_cannot_be_dismissed_here`.

`dismissInactive()` имеет отдельную safety-семантику и не должен заменяться `closeAfterManualReview()`:

- допускает только proven-terminal delivery `Rejected` / `Cancelled` либо `ProgramCompleted`;
- `TimedOut` намеренно исключён: потерянный JOB_ACK не доказывает, что Arduino idle;
- Running/Fault/ambiguous states не скрываются этим API;
- immutable snapshot/history не удаляются;
- метод не отправляет UART, не запускает двигатель и не управляет SSR.

`Tests/Web/check_job_cancel_recovery_contracts.js` также содержит явный контракт на этот узкий terminal-only dismiss и отдельно проверяет отсутствие `TimedOut` в разрешённом наборе.

## Regression runs

Четыре Actions-run показали две связанные ошибки после неправильного удаления API:

| Run | Commit | Failed gate | Root cause |
| --- | --- | --- | --- |
| `32578987033` | `778bc9f23e70631580c54273e16d11e218295f25` | ESP32 build | declaration `dismissInactive()` removed while implementation and caller still existed; compiler: no declaration matches |
| `32579000940` | `39142bd9834e26e349faf0ff4b2a6cb3076777f0` | ESP32 build | implementation also removed but `CM_StaticSiteServer.cpp` still called it; compiler: `JobStateStore` has no member `dismissInactive` |
| `32579052498` | `c1884d61e2eafbde4375d0e6d36d39dcc4bf6c4c` | host-tests | `check_job_cancel_recovery_contracts.js` still read the required `CM_JobStateDismiss.cpp`; ENOENT |
| `32579088543` | `5f70c834b2a508d600403049fbc81dfe31e83038` | host-tests | same ENOENT regression contract failure |

The Node 20 deprecation notices in these logs are warnings and are not the cause of failure.

## Corrective commits

```text
606816eac8e421ccbf52863ac8217698dcebf288
  Restore proven-inactive job dismiss API
  - restored public declaration in CM_JobStateStore.h
  - documented the narrow terminal-only semantics

dd6e6d1fb7dda2ba7ff7b16e702776bd0fd4d37b
  Restore proven-inactive job dismiss implementation
  - restored CM_JobStateDismiss.cpp
  - preserved explicit TIMED_OUT exclusion and fail-closed behavior

dfa4c601124dce1ad6728f93358ccdeb0e2946d3
  Correct job dismiss cleanup classification
  - corrected this handoff from DELETE to KEEP
```

No obsolete caller/test was removed merely to make CI pass; the production operator endpoint and its safety regression contract remain intact.

## Adjacent owner sweep

- `CM_WarehouseRepairValidation.cpp` — **KEEP**. `WarehouseStore::repairExists(...)` is used by manual warehouse writeoff validation.
- `CM_RepairCostingValidation.cpp` — **KEEP**. `RepairCostingWeb` calls its repair validation for costing/history APIs.
- `CM_WarehousePrice.cpp` — **KEEP**. It owns fail-closed warehouse price lookup.
- `CM_MaterialLedgerRepairReference.cpp` — **KEEP**. Material usage mutation validates the exact repair before writeoff.
- `CM_JobSpoolSelectionLookup.cpp` — **KEEP**. `WarehouseStore::confirmSpoolWriteOff()` and `confirmKgFirstWriteOff()` use `JobSpoolSelectionStore::loadReadOnly()` as exact source-session / repair / spool provenance gate before manual writeoff.
- `CM_WarehouseWriteOffLookup.cpp` — **KEEP** as the owner of exact-run duplicate protection through `confirmedWriteOffForSourceRun()`.
- obsolete `confirmedWriteOffForSourceSession()` — **DELETE completed** after full owner/API/test proof. Current write paths require exact `source_session_id + source_run_id`; `CM_WarehouseWriteOff.cpp` and `CM_WarehouseWriteOffWeb.cpp` use exact-run duplicate checks, reboot recovery parses the durable transaction itself, and `CM_WireWriteOffCoverageAudit` performs separate batched exact-run coverage for finalization. No production compatibility owner requires a session-only duplicate gate.

Exact-run-only cleanup commits:

```text
f7e75e2da82ca711b0085d38781354321b2b2311
  Remove obsolete session-only writeoff lookup API

89cdcae838335100fd24367cb45598a8fa0b9763
  Keep writeoff duplicate lookup exact-run only

42f75126b6c1549c9cd76607f72f28e25bed0bdb
  Protect exact-run-only writeoff duplicate lookup
```

`Tests/Web/check_writeoff_fault_contracts.js` now explicitly rejects reintroduction of `confirmedWriteOffForSourceSession()` and requires the public/store lookup to retain both exact source session and exact source run identity.

## Safety invariants preserved

- no automatic physical START;
- no automatic START between repeat cycles;
- no auto-resume after reboot;
- ESP32/Web do not directly control SSR;
- lost ACK/timeout does not prove Arduino idle;
- final repeat cannot reopen automatically;
- `RUN_COMPLETED` does not automatically deduct wire/material;
- manual writeoff remains tied to exact `source_session_id + source_run_id` and exact spool provenance when allocated;
- immutable run/snapshot/history evidence is preserved.

## Verification state

**GREEN / job-dismiss corrective recovery verified.**

Operator-provided GitHub Actions evidence confirms after restoration:

- `dd6e6d1fb7dda2ba7ff7b16e702776bd0fd4d37b` — `ESP32 Build` GREEN;
- `dd6e6d1fb7dda2ba7ff7b16e702776bd0fd4d37b` — `CMP Protocol Tests` GREEN;
- `dfa4c601124dce1ad6728f93358ccdeb0e2946d3` — `CMP Protocol Tests` GREEN.

Therefore the failed dismiss-removal runs listed above are retained only as regression history. The restored terminal-only dismiss owner is the accepted production state.

**Exact-run-only writeoff lookup cleanup is not yet recorded GREEN.** Fresh `ESP32 Build` + `CMP Protocol Tests` are required for the chain beginning at `f7e75e2d...` through `42f75126...` before its verification state is closed.

Cleanup rule reinforced by the dismiss incident: before deleting recovery/job-state APIs, verify not only direct C++ search results but also HTTP endpoint registration/inline lambdas and static regression-contract consumers. Empty indexed search is never sufficient proof.
