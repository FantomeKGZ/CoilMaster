# CoilMaster — next chat transfer

Date: **2026-08-24**  
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

Branch HEAD immediately before the frame-bound optimization was:

```text
66e5b84ac040b194ec5b169c1de8f1a5edad0845
docs(handoff): mark compact completion host tests green
```

Latest implementation commit:

```text
256b95754431c30b6da94bd43f399f5085e030fc
perf(uno): tighten Hall calibration frame buffer
```

Important earlier implementation/test commits:

```text
d77a24b3437831d0a236086055a193f233e1be7e  test(hall): align safety audit with compact completion
a928a51bc77c00407b146587aaf34c1e08a19998  docs: mark compact completion active
5d2b763f2e615d3444bdaed948e46f2eac22c0a9  feat(hall): switch Uno completion to compact frame
d35a6de9b378a48578274f4c7320d92b3be4b230  feat(hall): receive compact calibration completion
```

Always refetch branch HEAD before modifying anything.

## Verified CI — GREEN baseline

The user supplied two fresh post-fix Actions runs and both are fully successful:

```text
32753340348  host-tests  checkout d77a24b3437831d0a236086055a193f233e1be7e  SUCCESS
32753408620  host-tests  checkout b07de01ee4f3b1216153036dd977fa48bc053c2f  SUCCESS
```

The latest verified run `32753408620` passed all listed gates, including:

```text
CTest protocol tests
Hall calibration safety contracts
Hall lost-apply reconciliation
Uno Hall parser ownership
Hall raw migration ownership/wire contracts
Hall history and SD reference contracts
release safety / job lifecycle / material writeoff contracts
```

Hardware is **not** thereby declared GREEN.

The new frame-bound implementation commit `256b9575...` must not be called CI GREEN until its fresh Actions results are actually confirmed.

## Verified Uno build / memory baseline

Compact-completion Uno build:

```text
32751199627  build-uno  checkout a928a51bc77c00407b146587aaf34c1e08a19998  SUCCESS
RAM   1213 / 2048 = 59.2%   free 835 B
Flash 31640 / 32256 = 98.1% free 616 B
```

Comparison baseline before compact completion:

```text
checkout 02d9cd7e3c0679ae77d645a550af4f933b355e76
RAM   1213 / 2048 = 59.2%   free 835 B
Flash 31648 / 32256 = 98.1% free 608 B
```

Compact completion saves **8 B Flash**, RAM delta **0 B** versus that older baseline.

Do not claim a newer descendant Uno size until a newer exact build is measured.

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

Normal winding must remain autonomous on Uno after calibration is saved.

Never move realtime per-turn decision making to ESP32.

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

Uno no longer emits legacy `CAL_RESULT`.

For both receive paths, extended stats come only from `HallCalibrationRawCollector`; completion carries correlation identity only.

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

## Hall frame-bound proof completed

`HallCalibrationProtocol::MaxFrameLength = 96` was re-evaluated against every active Uno Hall TX formatter.

Exact worst-case wire lengths including `|CRC\n` are:

```text
CAL_APPLIED  76 B
CAL_SAMPLE   54 B
CAL_STATE    48 B
CAL_DONE     32 B
```

The longest frame is:

```text
CMP1|CAL_APPLIED|4294967295|PERSISTENCE_FAILED|1023|512|1000|FALLING|C|FFFF\n
```

This is **76 wire bytes**. `snprintf_P`/`appendCrc` also requires one byte for the terminating NUL, therefore the exact minimum safe formatter buffer is **77 bytes**.

Commit `256b9575...` changes:

```text
HallCalibrationProtocol::MaxFrameLength
96 -> 77
```

and records the proven `MaxAppliedWireLength = 76` with a compile-time `static_assert(MaxFrameLength == 77U)`.

This reduces each active Hall formatter stack frame using this local buffer by **19 bytes** compared with the prior 96-byte bound. Static global RAM usage may remain unchanged because these are automatic local buffers; exact build output is still required before claiming Flash/RAM deltas.

## Current optimization status

Keep:

- ESP32 extended aggregation active;
- compact `CAL_DONE` TX active;
- legacy ESP32 `CAL_RESULT` receive fallback active;
- parser/ownership/safety regressions from the previous checkpoint GREEN;
- Hall response buffer exact bound now 77 bytes in implementation commit `256b9575...`.

Rejected experiments not to repeat:

```text
manual CAL_SAMPLE formatter  -> worsened Flash by 454 B
bridge registration inlining -> 0 B gain
```

## Next work

Continue software optimization only; do not request intermediate physical hardware tests.

Immediate next steps:

1. Confirm fresh `build-uno` for `256b9575...`; record exact Flash/RAM versus `31640 / 1213`.
2. Confirm fresh host-tests descendant of `256b9575...`.
3. Then inspect/fix the known ESP32 semantic weakness in `CM_HardwareControlClient.cpp::processCalibrationApplied()`: APPLIED values must be range-validated before mirroring:
   - threshold `1..1023`;
   - hysteresis `1..512` and `< threshold`;
   - debounce `1..1000`;
   - direction must remain a recognized enum value.
4. Keep legacy CAL_RESULT receive parser unless compatibility removal is an explicit later decision.

Do not perform the final hardware acceptance until software optimization is finished.

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
