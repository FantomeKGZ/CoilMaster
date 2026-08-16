# CoilMaster — verification matrix for AI agents

This document answers: **what must be verified after a specific class of change.**

Do not claim a workflow is green until its actual run has completed successfully. A successful Git commit is not a build result.

## 1. Available automated gates

### Arduino Uno Build

Workflow:

```text
.github/workflows/arduino-uno-build.yml
```

Local equivalent:

```bash
pio run -e uno
```

Production sources selected by `platformio.ini`:

```text
Core/*.cpp
Arduino/*.cpp
firmware/arduino/src/main.cpp
```

### ESP32 Build

Workflow:

```text
.github/workflows/esp32-build.yml
```

Local equivalent:

```bash
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

Equivalent host commands:

```bash
cmake -S Tests/Protocol -B build/cmp-protocol
cmake --build build/cmp-protocol --parallel
ctest --test-dir build/cmp-protocol --output-on-failure
node Tests/Web/check_web_assets.js
node Tests/Web/check_release_contracts.js
node Tests/Web/check_final_acceptance_contracts.js
```

## 2. Verification matrix

| Change type | Arduino Build | ESP32 Build | CMP/Web audits | Hardware regression |
|---|---:|---:|---:|---:|
| Docs only | No | No | Usually no | No |
| `Tests/*` only | No unless source coupled | No unless source coupled | Yes for affected tests | No |
| Desktop/mobile/shared web only | No | ESP32 workflow may run because it watches `firmware/esp32/**`; C++ compile impact normally none | Yes | Only if runtime behavior needs device proof |
| ESP32 C++ service/API/storage | No | **Yes** | Usually **Yes** for public/data/safety contract | Targeted when persistence/network/hardware behavior changed |
| Arduino `Core/` or `Arduino/` | **Yes** | No unless protocol peer changed | Usually **Yes** | Targeted for machine/input/output behavior |
| UART/CMP1 wire contract | **Yes** | **Yes** | **Yes** | **Yes**, targeted cross-board regression |
| Physical START / SSR / Hall / machine state | **Yes** | Only if peer/service behavior changed | Relevant contract audits | **Mandatory targeted hardware test** |
| Workshop persisted schema | No | **Yes** | **Yes** | Persistence/reboot test when semantics changed |
| Warehouse/writeoff | No | **Yes** | **Yes** | Targeted exact-run/writeoff test when production logic changed |
| Backup/restore/apply | No | **Yes** | **Yes** | Targeted safe restore/reboot gate; destructive tests only disposable media |
| Network/FTP | No | **Yes** | Relevant Web contracts | Targeted device/network test when behavior changed |
| ESP32 hardware peripheral | No | **Yes** | As applicable | **Mandatory targeted hardware test** |
| Arduino hardware peripheral | **Yes** | No unless protocol coupled | As applicable | **Mandatory targeted hardware test** |
| Build/workflow config | Affected build | Affected build | Affected workflow | No unless production binary/behavior changes |

## 3. Hardware gates are change-scoped

CoilMaster v1 already passed the mandatory release hardware acceptance sequence.

Do **not** repeat the entire historical E2E suite for:

- documentation-only changes;
- test-only changes;
- unrelated static contract hardening.

Do re-run a closed hardware gate when the production code that established that gate changes.

Examples:

- changing `CM_StorageDiagnosticsWeb.*` → recheck storage diagnostics on device;
- changing motor import validation/persistence → recheck import + reboot persistence;
- changing transactional restore apply → recheck the relevant restore gate;
- changing Arduino START/SSR state logic → recheck physical START/safe SSR behavior;
- changing CMP1 framing → recheck ESP32↔Arduino communication.

## 4. Safety-contract verification

The following invariants are protected by repo audits and must also be considered during code review:

```text
physical START only
Arduino owns SSR
no automatic resume after reboot
RUN_COMPLETED does not auto-writeoff
manual exact spool/session/run writeoff
operator-only transactional restore
persisted stale restore evidence blocks operations
no automatic production-data cleanup
```

Primary automated guards:

```text
Tests/Web/check_release_contracts.js
Tests/Web/check_final_acceptance_contracts.js
```

When touching one of these contracts, update/extend the relevant assertion rather than relying only on prose documentation.

## 5. Protocol verification

Any change to `CMP1|...` must cover at least:

- valid frame;
- bad CRC;
- wrong field count;
- invalid numeric range;
- unknown status/type/capability;
- duplicate/stale identity where relevant;
- compatibility with the staged old/new peer behavior;
- timeout/retry behavior;
- no conversion of remote acceptance into physical start.

Run:

```text
CMP Protocol Tests
Arduino Uno Build
ESP32 Build
```

Then perform targeted two-board hardware verification if the wire behavior changed.

## 6. Persistence verification

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

A successful API response alone is not enough for a persistence change.

## 7. UI verification

For operator UI changes check both:

```text
firmware/esp32/web/desktop/
firmware/esp32/web/mobile/
```

And run:

```bash
node Tests/Web/check_web_assets.js
node Tests/Web/check_release_contracts.js
node Tests/Web/check_final_acceptance_contracts.js
```

UI should not:

- hide server errors;
- invent authoritative totals;
- imply physical motion from queued/accepted remote state;
- silently downgrade UNKNOWN/corrupted data;
- offer automatic destructive cleanup/restore behavior.

## 8. Backup/restore verification levels

### Repo-level only

Use for docs/tests/refactors that do not change runtime restore behavior.

### Device non-destructive

Use when status/preflight/inspection/safe-idle logic changes without intentionally corrupting data.

### Destructive fault injection

Only on:

```text
disposable microSD
or disposable filesystem image
```

Never intentionally corrupt or power-cut the working production microSD for a test.

## 9. Result labels

Use precise language:

```text
NOT VERIFIED       code/docs changed; relevant gate not run
FAILED             gate ran and failed
SUCCESS / GREEN    named workflow/test completed successfully
USER CONFIRMED     user explicitly verified real-device behavior
APPROVED           architecture/contract decision accepted
```

Do not transform missing CI visibility into `SUCCESS`.

## 10. Release-baseline rule

Current v1 release baseline is documented in:

```text
docs/PROJECT_HANDOFF/38_COILMASTER_V1_RELEASE_READY_2026-08-16.md
```

If a future change modifies production firmware/web behavior, the modified state is a **new candidate** for that affected scope until relevant automated and hardware regression gates pass.

Do not silently describe a new production commit as the old hardware-accepted baseline.

## 11. Before saying "done"

Confirm all applicable boxes:

```text
[ ] current target files were fetched before edit
[ ] current blob SHA used for existing-file update
[ ] source-of-truth branch is cmp-protocol-v1
[ ] ownership/lifecycle remains explicit
[ ] safety invariants preserved
[ ] persisted-data implications handled
[ ] desktop/mobile parity handled if relevant
[ ] automated gate actually passed or is explicitly NOT VERIFIED
[ ] hardware regression passed if required
[ ] AI project map/router updated if component topology changed
[ ] handoff/release docs updated only if project status materially changed
```
