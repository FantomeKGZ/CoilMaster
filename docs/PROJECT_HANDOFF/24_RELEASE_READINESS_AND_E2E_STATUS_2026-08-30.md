# Release readiness + physical E2E status — 2026-08-30

Repository: `FantomeKGZ/CoilMaster`  
Production/source-of-truth: `cmp-protocol-v1`  
Working branch: `arduino-ru-lcd-experiment`

## Authoritative correction

The older wording in `01_CURRENT_STATE.md`, `06_ACTIVE_WORK_AND_NEXT_STEPS.md`, and `14_NEXT_CHAT_TRANSFER_2026-08-30.md` that says the physical Arduino + ESP32 E2E still remains an outstanding gate is stale.

`13_HALL_RU_LCD_ACCEPTANCE.md` is authoritative for this point and already records an **operator-confirmed physical two-board E2E PASS** on the real CoilMaster. Hall/RU-LCD acceptance is COMPLETE for the current accepted hardware/firmware state unless a concrete hardware regression appears.

Do not require the same Hall/RU-LCD physical E2E again solely because older summary files still contain the previous wording.

## Latest exact verified CI

Current branch HEAD before this documentation commit:

```text
59c1e7e0a9e221706bdbb73985c7bd570fbb169d
CMP Protocol Tests #4632  run 33320805155 / SUCCESS
```

Immediately preceding verified checkpoints:

```text
5aeb8693554bf31225b0d0ee1f6d63254619dffe
CMP Protocol Tests #4631  run 33320763406 / SUCCESS

d84402251552522f60c2494da7dd7b19bb6af35a
CMP Protocol Tests #4630   run 33320670860 / SUCCESS
Arduino RU LCD Build #241 run 33320670903 / SUCCESS
```

The earlier CMP #4620 failure was intermediate; the later chain #4621 through #4632 recovered to SUCCESS.

Do not call this documentation commit itself GREEN until its own exact run is checked.

## Completed final Web/code audit blocks

The late experiment audit has now closed the previously identified final-completeness items, including:

- all desktop/mobile navigation and route parity sweep;
- Motor Import completeness and regression coverage;
- shared Web shell search/navigation consistency;
- FTP `/web` recovery including partial-bundle root-index failure;
- Wi-Fi profiles/static IP/network status/`coil.local` audit;
- backup/settings parity and stale settings text cleanup;
- stale/empty page sweep, including removal/absence of old `statistics.html`;
- service-job desktop/mobile pending-action parity;
- linked winding WORKING/STARTING canonical role parity;
- Hall Web duplicate-action lock and RU Web copy cleanup;
- persistent create single-flight guards for client, motor, repair, spool, and material.

Repeated-scan/storage optimization remains CLOSED / NO-CHANGE unless a measured bottleneck or concrete defect appears.

## Physical acceptance state

Authoritative physical evidence remains in `13_HALL_RU_LCD_ACCEPTANCE.md`:

- Arduino + ESP32 booted and worked normally;
- keypad remained responsive;
- normal RU LCD screens and Hall screens worked;
- Hall CGRAM restored correctly after exit;
- START remained local/Arduino-owned;
- SSR remained Arduino-owned/fail-safe;
- no automatic resume/start was introduced.

A new hardware test is required only if a concrete firmware/runtime regression is found, a safety-sensitive runtime path is changed, or for explicit final production-release acceptance requested by the operator.

## Production comparison

Current compare:

```text
base: cmp-protocol-v1 = 28c7917a906bc9b15736369e8986d0e0c354ab8c
head: arduino-ru-lcd-experiment
status: ahead
behind: 0
ahead: 864 commits
merge-base: 28c7917a906bc9b15736369e8986d0e0c354ab8c
```

This means the experiment branch contains the current production base linearly and has not fallen behind it.

This is **not** authorization to change production. Do not move or merge `cmp-protocol-v1` without a separate explicit user request.

## Current engineering state

No broad speculative audit is currently justified.

Remaining release work is now limited to:

1. concrete reproducible defects, if discovered;
2. exact CI verification of any new documentation/code commit;
3. final release verification / production promotion only after an explicit operator request;
4. optional final production acceptance E2E if the operator wants a release-candidate hardware pass after promotion planning.

## Safety invariants unchanged

- no automatic physical START/repeat START;
- no auto-resume after reboot;
- Arduino is sole SSR owner;
- ESP32/Web never directly control SSR;
- lost ACK/timeout does not prove Arduino idle;
- `RUN_COMPLETED` is evidence only;
- RUN_WIRE writeoff remains explicit/manual with exact `spool_id + source_session_id + source_run_id`;
- restore/recovery remains operator-controlled, transactional, fail-closed;
- mutation-time authoritative rereads and TOCTOU guards remain;
- append-only confirmed history is not silently edited/deleted;
- no premature DB/index migration;
- no automatic production-data truncation/rotation/deletion.
