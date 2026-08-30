# Release readiness + physical E2E status — 2026-08-30

Repository: `FantomeKGZ/CoilMaster`  
Production/source-of-truth: `cmp-protocol-v1`  
Working branch: `arduino-ru-lcd-experiment`

## Authoritative correction

The older wording in `01_CURRENT_STATE.md`, `06_ACTIVE_WORK_AND_NEXT_STEPS.md`, and `14_NEXT_CHAT_TRANSFER_2026-08-30.md` that says the physical Arduino + ESP32 E2E still remains an outstanding gate is stale.

`13_HALL_RU_LCD_ACCEPTANCE.md` is authoritative for this point and already records an **operator-confirmed physical two-board E2E PASS** on the real CoilMaster. Hall/RU-LCD acceptance is COMPLETE for the current accepted hardware/firmware state unless a concrete hardware regression appears.

Do not require the same Hall/RU-LCD physical E2E again solely because older summary files still contain the previous wording.

## Latest exact verified CI

Current pre-documentation release/package checkpoint:

```text
fa9b16113c60db0a5aaf5d18ec9e925600afbf90
CMP Protocol Tests #4636       run 33321775784 / SUCCESS
Arduino RU LCD Build #242      run 33321775525 / SUCCESS
```

Immediately preceding release-packaging checkpoint:

```text
a38dd2bb53c8af1b795b2cffd7099885791a7d39
CMP Protocol Tests #4635       run 33321756394 / SUCCESS
ESP32 Build #1811              run 33321756392 / SUCCESS
```

Previous release-readiness checkpoint:

```text
4059aec383c2022e61733ff3b60bd2c01ba15e2c
CMP Protocol Tests #4634       run 33321558476 / SUCCESS
```

Earlier verified checkpoints:

```text
7665238636e3e7c956486be6a93bd87090b8fcfa
CMP Protocol Tests #4633       run 33321339061 / SUCCESS

59c1e7e0a9e221706bdbb73985c7bd570fbb169d
CMP Protocol Tests #4632       run 33320805155 / SUCCESS

5aeb8693554bf31225b0d0ee1f6d63254619dffe
CMP Protocol Tests #4631       run 33320763406 / SUCCESS

d84402251552522f60c2494da7dd7b19bb6af35a
CMP Protocol Tests #4630       run 33320670860 / SUCCESS
Arduino RU LCD Build #241      run 33320670903 / SUCCESS
```

The earlier CMP #4620 failure was intermediate; the later chain #4621 through #4636 recovered and remained SUCCESS.

## Promotion build-chain evidence

The last Web/runtime commit before the final regression-only/docs/CI-packaging commits is:

```text
9430ea189ab1ed45480180f06a1af88e821b31dc
CMP Protocol Tests #4629       run 33320651036 / SUCCESS
ESP32 Build #1810              run 33320651041 / SUCCESS
Arduino RU LCD Build #240      run 33320651027 / SUCCESS
```

The next commit `d84402251552522f60c2494da7dd7b19bb6af35a` changes only `Tests/Web/check_crud_page_separation.js` and is independently verified by CMP #4630 and Arduino RU LCD #241.

Exact compare from `d8440225...` to `76652386...` shows only three later commits and only `docs/PROJECT_HANDOFF` changes. Therefore no firmware, ESP32 runtime, Arduino runtime, Web asset or test source changed after the verified code/test checkpoint before the release-packaging workflow changes.

The later commits `a38dd2bb...` and `fa9b1611...` change only GitHub Actions packaging so successful builds persist firmware artifacts; they do not change firmware/Web runtime behavior.

## Release artifacts

Release packaging is now CI-persisted and verified.

ESP32 workflow:

```text
head: a38dd2bb53c8af1b795b2cffd7099885791a7d39
ESP32 Build #1811 / run 33321756392 / SUCCESS
artifact id: 9735092955
artifact: esp32-firmware-a38dd2bb53c8af1b795b2cffd7099885791a7d39
size: 11919337 bytes
retention: 30 days
```

The workflow packages:

- `.pio/build/esp32/bootloader.bin`
- `.pio/build/esp32/partitions.bin`
- `.pio/build/esp32/firmware.bin`
- `.pio/build/esp32/firmware.elf`

Arduino RU LCD comparison/release workflow:

```text
head: fa9b16113c60db0a5aaf5d18ec9e925600afbf90
Arduino RU LCD Build #242 / run 33321775525 / SUCCESS
artifact id: 9735100426
artifact: arduino-ru-lcd-firmware-fa9b16113c60db0a5aaf5d18ec9e925600afbf90
size: 12056544 bytes
retention: 30 days
```

Its artifact packages:

- normal Uno `firmware.hex` + `firmware.elf`
- RU Uno `firmware.hex` + `firmware.elf`
- matching ESP32 `bootloader.bin`, `partitions.bin`, `firmware.bin`, `firmware.elf`
- Uno / RU Uno / ESP32 build logs

The `/web` SD bundle is intentionally not duplicated into these firmware workflows. It remains owned by the separate `reference-legacy-import` -> `reference-sd-release` production pipeline, whose release step is intentionally gated on a successful `cmp-protocol-v1` source run after production promotion.

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

The release-packaging changes above are CI-only and do not require another physical hardware acceptance run.

## Production comparison

Current compare before this documentation update:

```text
base: cmp-protocol-v1 = 28c7917a906bc9b15736369e8986d0e0c354ab8c
head: arduino-ru-lcd-experiment = fa9b16113c60db0a5aaf5d18ec9e925600afbf90
status: ahead
behind: 0
ahead: 868 commits
merge-base: 28c7917a906bc9b15736369e8986d0e0c354ab8c
```

This means the experiment branch contains the current production base linearly and has not fallen behind it.

This is **not** authorization to change production. Do not move or merge `cmp-protocol-v1` without a separate explicit user request.

## Current engineering state

Repo-side release-readiness, firmware build verification, and release-artifact persistence are complete for the current release candidate.

Remaining release work is limited to:

1. concrete reproducible defects, if discovered;
2. exact CI verification of any new documentation/code commit;
3. production promotion only after an explicit operator request;
4. after promotion, verify production CI and the production-owned reference `/web` SD release pipeline;
5. optional final production acceptance E2E only if the operator explicitly requests another release-candidate hardware pass.

Do not start another broad speculative audit without a concrete defect or a new product requirement.

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
