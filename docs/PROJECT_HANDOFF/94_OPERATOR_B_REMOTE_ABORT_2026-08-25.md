# CoilMaster — Physical B operator abort

Дата: **2026-08-25**  
Ветка: **`cmp-protocol-v1`**

## Hardware acceptance finding

During final two-board hardware acceptance the operator confirmed that keypad `B` did not leave an active winding. This was unsafe operationally for an incorrectly entered/sent program because the operator could be forced to finish or mechanically undo a wrong winding before starting the corrected one.

## Required behavior

`B` is now an explicit physical operator abort for an ESP32-owned active job in READY/WINDING/PAUSED/MANUAL/COIL_COMPLETE states:

1. Arduino Uno removes SSR authority first with `ssr.forceOff()`.
2. Uno cancels the local machine job and returns the local UI to HOME.
3. Uno sends an exact-job-id CRC-protected `JOB_CANCEL_ACK` with detail `OPERATOR_ABORT`.
4. ESP32 accepts this unsolicited operator abort only when the exact job id equals its last queued job id.
5. If a run had already started, ESP32 closes the active job lifecycle as `ClosedAfterReview` while preserving existing `lastRunId` / completed-run evidence.
6. No `RUN_COMPLETED` is fabricated and no wire/material writeoff occurs automatically.

Ordinary Web/ESP32 `JOB_CANCEL` semantics remain unchanged: a started run is not remotely erased through the normal cancel channel.

## Implementation

```text
daab34ca733a2b082440af22f006a2db7f433f1a
fix(runtime): allow physical B operator abort

534109da70f0856a31d89399b2e821e0169d6933
ci: verify operator B abort on both boards
```

Changed production areas:

- `firmware/arduino/src/main.cpp`
- `firmware/esp32/src/CM_UartEventReceiver.cpp`
- `firmware/esp32/src/CM_JobStateRemoteCancel.cpp`
- `Tests/Web/check_job_cancel_recovery_contracts.js`

The one-shot patch workflow used to apply the exact-SHA patch was removed immediately afterwards and is not part of the production workflow set.

## Verified CI

Verified on descendant head `534109da70f0856a31d89399b2e821e0169d6933`:

```text
CMP Protocol Tests #3124 / run 32835460810 / SUCCESS
Arduino Uno Build #247 / run 32835460802 / SUCCESS
ESP32 Build #1444 / run 32835460847 / build job SUCCESS
```

Uno resource result after the fix:

```text
RAM   1221 / 2048 = 59.6%   free 827 B
Flash 31598 / 32256 = 98.0%  free 658 B
```

The required CI safety margin remains satisfied (`>=512 B` RAM and Flash headroom).

## Hardware re-test required

Reflash both boards from current `cmp-protocol-v1`, then test one intentionally small/wrong ESP32-owned job:

```text
send JOB
-> verify no auto-start
-> physical START
-> verify winding begins / RUN_STARTED exists
-> press B
-> SSR/motor stops immediately
-> Uno returns HOME
-> ESP32 closes active job and allows corrected next job
-> no RUN_COMPLETED fabricated
-> no automatic wire/material writeoff
-> prior RUN_STARTED/run_id evidence remains
```

After this operator-abort test passes, resume the remaining final two-board hardware acceptance sequence.
