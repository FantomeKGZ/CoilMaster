# CoilMaster — next chat transfer

Date: **2026-08-25**  
Repository: `FantomeKGZ/CoilMaster`  
Source of truth: **`cmp-protocol-v1` only**. Never use `main` as source.

## Start here

This file is the current transfer checkpoint for a new chat. Read it before older numbered handoffs.

Then read, in order:

```text
/AGENTS.md
docs/PROJECT_HANDOFF/00_READ_FIRST.md
docs/PROJECT_HANDOFF/72_HALL_COMPACT_COMPLETION_ACTIVE_2026-08-24.md
docs/PROJECT_HANDOFF/71_HALL_RAW_STREAM_MIGRATION_2026-08-24.md
docs/PROJECT_HANDOFF/70_HALL_CALIBRATION_HISTORY_2026-08-24.md
docs/PROJECT_HANDOFF/69_ARDUINO_UNO_MINIMAL_RUNTIME_PLAN_2026-08-24.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
docs/PROJECT_HANDOFF/03_PROTOCOL_AND_WINDING_FLOW.md
```

If older Hall text conflicts with `72` or this file, this file wins.

## Current branch checkpoint

Frame-bound implementation:

```text
256b95754431c30b6da94bd43f399f5085e030fc
perf(uno): tighten Hall calibration frame buffer
```

ESP32 applied-profile validation:

```text
a2bd5115a75b530f9f72c5e32f5a72e0e847d9d2
fix(hall): validate applied settings before mirror

ed32e55f91b5e6bada33315c2daafab1e7cd5e6c
test(hall): require applied range validation
```

ESP32 authoritative settings-state validation:

```text
fb547c6c3a5537d42839a6147309985ae717f4e8
fix(hall): validate settings state before mirror

56268ab17c888510a9ed2b0aa7461b8ef3b250e9
test(hall): require settings state range validation
```

ESP32 Hall telemetry semantic validation:

```text
a18158da211ab3e36fe043f80e278c252d184312
fix(hall): validate telemetry semantics before mirror

94ec3d6749debc09d5406eb49612125f060c4611
test(hall): require telemetry semantic validation
```

ESP32 direct settings send fail-fast validation:

```text
f44e9a8966e2f6f8f385e0c718c2c952e4d6c45a
fix(hall): reject invalid settings before send

09de40da0c8a2f1f036f1c7624ee260f386cf2ea
test(hall): require settings send validation
```

ESP32 CFG reply correlation hardening:

```text
f927b9a41ea020237cb354e3bbd80d302e799f64
fix(hall): correlate cfg replies to pending request

e4efdb3835d5a059bd4dae70b9883587036f2634
test(hall): require cfg reply correlation
```

Important earlier implementation/test commits:

```text
d77a24b3437831d0a236086055a193f233e1be7e  test(hall): align safety audit with compact completion
a928a51bc77c00407b146587aaf34c1e08a19998  docs: mark compact completion active
5d2b763f2e615d3444bdaed948e46f2eac22c0a9  feat(hall): switch Uno completion to compact frame
d35a6de9b378a48578274f4c7320d92b3be4b230  feat(hall): receive compact calibration completion
```

Always refetch branch HEAD before modifying anything.

## Verified CI — GREEN

Verified baseline and Hall hardening runs supplied by the user:

```text
32759806141  build-uno    checkout 256b95754431c30b6da94bd43f399f5085e030fc  SUCCESS
32760002181  host-tests   checkout 1e8328733f4495f023748ed7c4f34afdf6f7168e  SUCCESS
32760501006  build-esp32  checkout a2bd5115a75b530f9f72c5e32f5a72e0e847d9d2  SUCCESS
32760592287  host-tests   checkout ed32e55f91b5e6bada33315c2daafab1e7cd5e6c  SUCCESS
32760645056  host-tests   checkout a8e64695be3413081b71cf052ee361aff0669379  SUCCESS
32761464209  build-esp32  checkout fb547c6c3a5537d42839a6147309985ae717f4e8  SUCCESS
32761464222  host-tests   checkout fb547c6c3a5537d42839a6147309985ae717f4e8  SUCCESS
32761539190  host-tests   checkout 56268ab17c888510a9ed2b0aa7461b8ef3b250e9  SUCCESS
32761611196  host-tests   checkout 89741c5f1582c295cabcab08c06d7e91ff3c3bf7  SUCCESS
32762099225  build-esp32  checkout a18158da211ab3e36fe043f80e278c252d184312  SUCCESS
32762099263  host-tests   checkout a18158da211ab3e36fe043f80e278c252d184312  SUCCESS
32762184024  host-tests   checkout 94ec3d6749debc09d5406eb49612125f060c4611  SUCCESS
32762259476  host-tests   checkout 1c7cf1aa4943285f26cd79683ccfdb15ac9e8ca1  SUCCESS
32763177751  host-tests   checkout 728a9e79f0bf69d6cd1a45c9fd2932b2b50d32ec  SUCCESS
32792203041  build-esp32  checkout f44e9a8966e2f6f8f385e0c718c2c952e4d6c45a  SUCCESS
32792203111  host-tests   checkout f44e9a8966e2f6f8f385e0c718c2c952e4d6c45a  SUCCESS
32792249748  host-tests   checkout 09de40da0c8a2f1f036f1c7624ee260f386cf2ea  SUCCESS
32792288954  host-tests   checkout 9719b08dbf0e26a1698194651854ae050a5bb530  SUCCESS
```

The host runs passed all CTest targets and every listed contract audit, including Hall calibration safety, lost-apply reconciliation, Uno Hall parser ownership, Hall raw migration, Hall history, release safety, job lifecycle and material writeoff contracts.

Therefore `CAL_APPLIED`, `CFG_STATE`, `HALL_STATE`, direct `CFG_SET`, compact completion and legacy-audit blocks are verified GREEN at the software CI level.

The newer CFG reply-correlation commits `f927b9a4...` / `e4efdb38...` are implemented and contract-covered but must not be called GREEN until fresh Actions complete successfully.

Hardware is **not** thereby declared GREEN.

## Verified Uno build / memory

Fresh frame-bound Uno build:

```text
32759806141  build-uno  checkout 256b95754431c30b6da94bd43f399f5085e030fc  SUCCESS
RAM   1213 / 2048 = 59.2%   free 835 B
Flash 31640 / 32256 = 98.1% free 616 B
```

Tightening `HallCalibrationProtocol::MaxFrameLength` from `96` to `77` changes static reported Flash/RAM by **0 B / 0 B** versus the compact-completion baseline and reduces each active Hall formatter automatic stack buffer by **19 bytes**.

## Active Hall architecture

Arduino Uno remains authoritative for realtime/safety minimum:

- physical Hall A0 read;
- normal-winding threshold/hysteresis/debounce/direction;
- realtime turn counting;
- physical START;
- SSR ownership/control;
- local calibration confirmation;
- calibration peer timeout/fail-closed abort;
- final Hall settings EEPROM;
- exact proposal measurement-id gate;
- local confirmation before EEPROM apply.

ESP32 owns extended Hall test/calibration analysis:

- raw sample collection;
- baseline average;
- min/max/span/sample count/duration;
- recommendation threshold/hysteresis/direction;
- Web/UI/status/history;
- proposal orchestration/reconciliation;
- settings mirror/audit.

Normal winding must remain autonomous on Uno after calibration is saved. Never move realtime per-turn decision making to ESP32.

## Active Hall wire path

Raw stream:

```text
CMP1|CAL_SAMPLE|BASELINE|raw|sequence|elapsed_ms|C|CRC
CMP1|CAL_SAMPLE|RUN|raw|sequence|elapsed_ms|C|CRC
```

Completion emitted by Uno is compact:

```text
CMP1|CAL_DONE|measurement_id|C|CRC
```

ESP32 active compact path:

```text
CAL_DONE
 -> HallCalibrationDoneProtocol::parseDone
 -> HallCalibrationCompletionAdapter::buildFromDone
 -> HallCalibrationRemoteResult
 -> HallCalibrationAnalyzer / Web flow
```

ESP32 intentionally keeps backward-compatible legacy receive fallback:

```text
CAL_RESULT
 -> HardwareControlClient::processCalibrationResult
 -> UartEventReceiver::takeHallCalibrationResult
 -> HallCalibrationCompletionAdapter::enrichLegacy
```

Uno no longer emits legacy `CAL_RESULT`. A targeted audit found no authoritative active `CAL_RESULT|VALID` emitter in current `cmp-protocol-v1`, so the receive fallback remains unchanged for compatibility rather than being narrowed without an authoritative legacy contract.

## Hall calibration flow / safety

Current intended flow remains:

```text
ESP32 CAL_ARM
 -> Uno WAITING_LOCAL_CONFIRM
 -> local # confirmation
 -> ARMED_WAITING_START
 -> baseline collection
 -> separate physical START
 -> RUNNING
 -> CAL_DONE + raw summary on ESP32
 -> analyzer recommendation
 -> CAL_PROPOSAL exact measurement_id
 -> WAITING_APPLY_CONFIRM
 -> local # confirmation
 -> EEPROM apply
```

Safety invariants never weaken:

- no automatic physical START;
- no automatic START between repeat cycles;
- no auto-resume after reboot;
- Arduino owns SSR;
- ESP32/Web never directly controls SSR;
- peer timeout during calibration fails closed / aborts / SSR OFF;
- START while waiting apply confirmation neither saves nor starts;
- proposal never auto-applies;
- proposal is RAM-only until local confirmation;
- lost `CAL_APPLIED` never triggers proposal replay;
- `CFG_GET` reconciliation may refresh authoritative settings but value equality does not prove exact apply event.

## Production invariants outside Hall

Keep intact:

- remote JOB remains READY until physical START;
- only physical START creates `RUN_STARTED` and permits SSR through Arduino;
- exact `RUN_COMPLETED` retry-until-ACK behavior;
- lost ACK/timeout never proves Arduino idle;
- final repeat cannot auto-reopen;
- `RUN_COMPLETED` never performs automatic material writeoff;
- material writeoff is manual and exact `source_session_id + source_run_id + immutable spool_id`;
- cancellation never erases immutable history;
- reboot never auto-resumes winding/calibration/restore/apply.

## ESP32 Hall validation status

`processCalibrationApplied()` and `processSettingsState()` reject invalid mirrored settings:

```text
threshold           1..1023
hysteresis          1..512 and < threshold
releaseDebounceMs   1..1000
direction           RISING or FALLING
```

`processTelemetryState()` rejects invalid `HALL_STATE` telemetry unless:

```text
rawAdc/windowMin/windowMax  0..1023
windowMin <= rawAdc <= windowMax
threshold                   1..1023
hysteresis                  1..512 and < threshold
releaseDebounceMs           1..1000
sampleCount                 > 0
releaseBoundary             exactly matches threshold/hysteresis/direction
```

`setSettings()` applies the same Hall profile bounds before formatting/queueing `CMP1|CFG_SET|HALL|...`. Uno remains authoritative and still validates every received `CFG_SET` through `HardwareSettings::isValid()`.

`processSettingsResult()` now also correlates generic `CFG_ACK/CFG_NACK` to the active request lane:

- a reply is ignored unless the request has actually been sent and is waiting for a reply;
- direct CFG replies are accepted only for SET/RESET/telemetry start/stop;
- GET settings and calibration ARM/ABORT/GET cannot be completed by stale CFG replies;
- a staged `CAL_PROPOSAL` accepts only the early generic `CFG_NACK|BUSY` path used by Uno while calibration is already active;
- once `WAITING_APPLY_CONFIRM` is observed, generic CFG replies cannot terminate the staged proposal; completion remains `CAL_APPLIED` or the existing state/reconciliation flow.

This is the strongest correlation available without changing the existing generic CFG reply wire format to carry a request id.

## Current optimization status

Keep:

- ESP32 extended aggregation active;
- compact `CAL_DONE` TX active;
- legacy ESP32 `CAL_RESULT` receive fallback active;
- Hall response buffer exact bound 77 bytes, build verified;
- `CAL_APPLIED` range validation verified ESP32-build GREEN + host-tests GREEN;
- `CFG_STATE` range validation verified ESP32-build GREEN + host-tests GREEN;
- `HALL_STATE` semantic validation verified ESP32-build GREEN + host-tests GREEN;
- direct `CFG_SET` fail-fast validation verified ESP32-build GREEN + host-tests GREEN;
- CFG reply correlation implemented and contract-covered, fresh CI pending;
- no intermediate hardware test requested during this optimization phase.

Rejected experiments not to repeat:

```text
manual CAL_SAMPLE formatter  -> worsened Flash by 454 B
bridge registration inlining -> 0 B gain
```

## Next work

Continue software optimization only; do not request intermediate physical hardware tests.

Immediate next steps:

1. Confirm fresh ESP32 build + host-tests for `f927b9a4...` / `e4efdb38...` or a descendant.
2. After GREEN, continue one more targeted Hall request/reply semantic review; do not reopen broad cleanup.
3. Keep legacy CAL_RESULT receive parser unless compatibility removal is an explicit later decision.
4. Do not perform the final hardware acceptance until software optimization is finished.

## Final hardware acceptance gate — later only

When optimization is complete, one full physical acceptance should cover:

1. Uno boot/home, no reset loop.
2. LCD 1602.
3. Keypad `1`, `#`, `*`, `D`, emergency `D * # D`.
4. Hall manual rotation without SSR.
5. `CAL_ARM -> # -> baseline -> physical START`.
6. UART disconnect during calibration -> ABORTED, SSR OFF.
7. ESP result -> exact proposal -> WAITING_APPLY_CONFIRM -> local `#` -> EEPROM.
8. START while WAITING_APPLY_CONFIRM neither saves nor starts.
9. Reboot: no calibration/proposal resume; last CRC-valid profile loaded.
10. Remote JOB stays READY until physical START.
11. Only physical START creates RUN_STARTED and permits SSR through Arduino.
12. Exact RUN_COMPLETED retries until ACK.
13. Material writeoff manual/exact session/run/spool.

## Repository work rules for next chat

- Work only on `cmp-protocol-v1`.
- Before updating an existing file: fetch current branch version and current blob SHA.
- Before creating a new file: verify exact path does not exist.
- Never infer GREEN from empty combined-status output.
- Distinguish host-tests, Uno compile, ESP32 compile, memory and physical hardware verification.
- Keep PROJECT_HANDOFF current after meaningful blocks.
- Do not auto-clear EEPROM while troubleshooting.
- Do not ask for hardware tests during the current optimization phase.
