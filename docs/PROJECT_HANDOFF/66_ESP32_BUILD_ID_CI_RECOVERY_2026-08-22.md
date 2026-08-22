# Checkpoint 66 — ESP32 build identity / CI recovery

Date: 2026-08-22  
Branch: `cmp-protocol-v1`

## Why this checkpoint exists

Seven consecutive red runs supplied by the operator were investigated directly:

```text
32548188235 — ESP32 Build #1263 — 2e22980f6de4cdb0b482b7d19b963fc8b37dbcb9
32548207448 — ESP32 Build #1264 — 295a795092ebaf11cba203e289ea2dc2cfac3f61
32548227852 — ESP32 Build #1265 — 684a765462f2d42202ded4154058d67a8ca9a2a6
32548296438 — ESP32 Build #1266 — e94f3022ebb22e271c52e5671083a93397c1cf0a
32548374877 — ESP32 Build #1267 — 9e6937106eaae63994fbab468d17745e779162aa
32548464894 — ESP32 Build #1268 — 7f2813b8c3e32f81911604437aaf19a85ccf2faf
32548673892 — ESP32 Build #1269 — 5c06538a4781f57a83a1884827b3bc5a3033ef03
```

All seven are the same workflow (`ESP32 Build`) on sequential commits. Logs from the earliest and latest runs show the same inherited compile failure in `CM_StaticSiteServer.cpp` while expanding Phase 9 firmware build identity macros.

## Confirmed root cause

`scripts/platformio_build_id.py` passed string metadata through `CPPDEFINES` using escaped quotes. SCons/compiler command-line processing removed the effective C-string quoting, causing values such as:

```text
git SHA / -dirty suffix
cmp-protocol-v1
2026-08-22T03:...
```

to be parsed as C++ tokens/numeric literals.

Representative errors:

```text
invalid digit in octal constant
unable to find numeric literal operator
'dirty' was not declared
'cmp' was not declared
'protocol' was not declared
'v1' was not declared
```

This failure was unrelated to the UART timeout/recovery fixes themselves; those commits inherited the already-broken build identity mechanism.

## Fix

### Safe build metadata transport

Commit:

```text
c07c188a2a429cd68cb7fc8d1925e90a5d789cc9
Generate build identity header safely
```

The build script now:

1. obtains SHA/branch/build UTC as before;
2. generates `CM_BuildIdentityGenerated.h` inside PlatformIO `$BUILD_DIR`;
3. writes proper C string literals into that private generated header;
4. adds `$BUILD_DIR` to include paths;
5. force-includes the generated header with compiler `-include`;
6. no longer passes build identity strings through `CPPDEFINES`.

`CM_StaticSiteServer.cpp` keeps its existing `unknown` fallback macros, so non-PlatformIO/static analysis remains safe.

Regression guard:

```text
3d63dc281d40070d90b42fbc7b3a202ab8f27f14
Guard build identity header generation
```

`Tests/Web/check_shared_app_shell_contracts.js` now forbids build identity string `CPPDEFINES` and requires the generated-header/force-include contract.

## CI trigger gap found during recovery

`.github/workflows/esp32-build.yml` previously watched only:

```text
firmware/esp32/**
platformio.ini
workflow file
```

Therefore a build-script-only change to `scripts/platformio_build_id.py` did **not** trigger `ESP32 Build`. The workflow also missed `Shared/**`, even though ESP32 production code compiles shared protocol/CRC headers.

Fix:

```text
5fa8c89250c517f57ab61f24f3e61ebc98931234
Cover ESP32 build inputs in CI
```

ESP32 Build now triggers on both:

```text
Shared/**
scripts/platformio_build_id.py
```

for push and pull request paths.

## Related audit work completed while CI recovery was in progress

B-003 network HTTP/storage semantics was also fixed:

```text
1048dd922d059b3f8ec0f31fafc3bd24795688af
Separate network validation from storage faults

5c05ad2f636ff5548ca80317010346dd66ad1af5
Guard network API error semantics
```

Network API now distinguishes validation/not-found/capacity from persistence/store/manager failures instead of reporting storage failures as `400` operator-input errors.

## Verification status

At checkpoint creation:

```text
Old ESP32 runs #1263..#1269: FAILED — exact root cause confirmed
Other visible workflows: USER CONFIRMED GREEN by operator
New ESP32 Build after build-identity/trigger fixes: NOT VERIFIED yet
```

Do not mark the current implementation GREEN until a fresh `ESP32 Build` completes successfully after `5fa8c892...` (or a later descendant containing the same fixes).

## What to do after fresh ESP32 GREEN

Return immediately to the already-open full-code audit, not to generic Phase 9/UART work:

```text
B-002 — P1 — global restore/apply production-mutation interlock
then remaining ESP32 persistence/network/FTP/RTC/SD/backup audit
then Web audit
then Tests/CI audit
then docs/AI audit
then final cross-layer recheck
```

Authoritative active queue remains:

```text
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
```
