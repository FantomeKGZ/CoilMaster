# CoilMaster — change router for AI agents

Use this document when the task is known but the implementation location is not.

For every route below, **fetch the listed files from `cmp-protocol-v1` immediately before editing**. The list is a starting set, not permission to ignore current call sites.

## 1. Change physical START, SSR, Hall, keypad, LCD or buzzer

Open first:

```text
firmware/arduino/src/main.cpp
Arduino/Config/CM_Pins.h
Core/
Arduino/CM_SsrController.*
Arduino/CM_HallTurnSource.*
Arduino/CM_DebouncedButton.*
Arduino/CM_Lcd1602View.*
Arduino/CM_Buzzer*.h/.cpp
```

Also inspect the current state-machine/input path in `Core/`.

Safety impact:

- physical START must remain physical;
- SSR authority stays on Arduino;
- errors/lost communication must fail safe;
- reboot must not auto-resume movement.

Verification:

- Arduino Uno build;
- relevant host tests;
- targeted hardware test for physical behavior.

## 2. Change UART/CMP1 frame, field, CRC, ACK/NACK or retry behavior

Open first:

```text
Arduino/CM_UartEventTransport.h
Arduino/CM_UartEventTransport.cpp
firmware/esp32/src/CM_UartEventReceiver.h
firmware/esp32/src/CM_UartEventReceiver.cpp
Shared/CMP1Text/CM_Cmp1Crc.h
Tests/Protocol/
Tests/CMP1Text/
docs/PROJECT_HANDOFF/03_PROTOCOL_AND_WINDING_FLOW.md
```

Then search exact message names in both board entrypoints.

Required questions:

- Is wire compatibility preserved?
- What does an old peer do with the new frame?
- Are field count/length/range/CRC checks still strict?
- Are retries bounded?
- Does any changed ACK accidentally become a physical-start permission?

Verification:

- CMP Protocol Tests;
- Arduino Uno Build;
- ESP32 Build;
- targeted hardware UART regression when wire behavior changes.

Do not use `Shared/Protocol/` as a shortcut; it is the older binary protocol.

## 3. Add or change an ESP32 HTTP API endpoint

Open first:

```text
firmware/esp32/src/main.cpp
firmware/esp32/src/CM_StaticSiteServer.h
firmware/esp32/src/CM_StaticSiteServer.cpp
```

Then open the owning domain `*Web.h/.cpp` plus the domain store/service it calls.

Pattern:

```text
WebServer route → strict request validation → domain operation/read → authoritative response
```

Check:

- who owns the `*Web` object;
- where routes are registered;
- method/path/status/error semantics;
- max request/response sizes;
- fail-closed behavior;
- whether the endpoint mutates production data;
- whether UI uses the same contract;
- whether Web audits need a new contract assertion.

Verification:

- ESP32 Build for C++ changes;
- `Tests/Web/` audits for public route/UI contract changes;
- hardware check only when runtime/device behavior actually requires it.

## 4. Add or change a web page / operator UI

Open first:

```text
firmware/esp32/web/desktop/
firmware/esp32/web/mobile/
firmware/esp32/web/shared/
Tests/Web/check_web_assets.js
Tests/Web/check_release_contracts.js
Tests/Web/check_final_acceptance_contracts.js
```

Then inspect the API route used by the page.

Default parity rule:

- implement meaningful operator functionality in both desktop and mobile;
- move common logic to `web/shared/` when practical;
- update navigation/links for both variants;
- keep server truth authoritative;
- show server errors instead of hiding them.

Never make UI state imply that a physical action happened before the confirmed hardware event.

## 5. Change clients, motors or repairs

Open first:

```text
firmware/esp32/src/CM_RepairRegistry.h
firmware/esp32/src/CM_RepairRegistry.cpp
firmware/esp32/src/CM_RepairRegistryWeb.h
firmware/esp32/src/CM_RepairRegistryWeb.cpp
firmware/esp32/src/CM_RepairRegistryLookupWeb.*
firmware/esp32/src/CM_MotorSimilarityWeb.*
```

Data root:

```text
/data/workshop/
```

If changing a persisted record shape, also inspect:

```text
CM_WorkshopPersistenceIntegrityAudit.*
CM_BackupBusinessDataIntegrityAudit.*
CM_FlatJsonObjectValidator.h
backup/restore whitelist or plan code that covers workshop data
```

For motor import also inspect:

```text
docs/MOTOR_IMPORT_FORMAT.md
firmware/esp32/web/desktop/motor-import.html
firmware/esp32/web/mobile/motor-import.html
```

Compatibility rule: old records must remain intentionally readable or have an explicit migration plan.

## 6. Change linked winding job creation

Open first:

```text
firmware/esp32/src/main.cpp
CM_JobLinkageRequest.*
CM_JobLinkageResolver.*
CM_JobSnapshotStore.*
CM_JobSpoolSelectionStore.*
CM_JobSpoolSelectionWeb.*
CM_JobStateStore.*
CM_PersistentIdAllocator.*
CM_WindingProgramParser.h
```

Core contract:

```text
repair + motor + exact ACTIVE spool
→ immutable snapshot/spool selection
→ UART job delivery
→ physical START later on Arduino
```

Do not loosen exact spool identity or turn job acceptance into physical start.

## 7. Change RUN_STARTED / RUN_COMPLETED journaling or winding history

Open first:

```text
CM_UartEventReceiver.*
CM_WindingJournal.*
CM_WindingJournalQuery.h
CM_WindingJournalQuery.cpp
CM_WindingJournalQueryValidation.cpp
CM_WindingJournalWeb.*
CM_WindingPersistenceIntegrityAudit.*
CM_WindingJournalTransitionAudit.*
```

Authoritative journal:

```text
/data/winding-runs/events.ndjson
```

Important:

- full-file validation uses `WindingJournalQuery::validateAll()`;
- transition audit is separate;
- do not reintroduce cursor-pagination as authoritative full validation;
- `RUN_COMPLETED` itself must never write off wire.

## 8. Change wire spool warehouse, pricing or movements

Open first by prefix:

```text
firmware/esp32/src/CM_Warehouse*.h/.cpp
```

For writeoff specifically inspect:

```text
CM_WarehouseWriteOffWeb.*
CM_JobSpoolSelectionStore.*
CM_WindingJournalQuery*
```

Required exact provenance:

```text
spool_id + source_session_id + source_run_id
```

Rules:

- writeoff remains manual;
- persisted exact spool selection must match;
- completed run must be proven;
- duplicate exact-run writeoff remains rejected;
- historical price/material snapshot must not be recomputed from current values.

Also inspect:

```text
CM_WarehousePersistenceIntegrityAudit.*
CM_WarehouseMovementIntegrityAudit.*
CM_BackupBusinessDataIntegrityAudit.*
```

## 9. Change auxiliary materials or material usage

Open first by prefix:

```text
firmware/esp32/src/CM_Material*.h/.cpp
```

Then inspect repair costing/pricing integration.

Persistence concerns:

- material exists and is active;
- repair exists;
- currency policy is explicit;
- quantity/stock validated before commit;
- line cost snapshot persisted;
- pending/recovery semantics preserved where multi-step.

Integrity:

```text
CM_MaterialPersistenceIntegrityAudit.*
```

## 10. Change costing, finalization or pricing

Open first by prefix:

```text
CM_RepairCosting*.h/.cpp
CM_RepairPricing*.h/.cpp
```

Then inspect:

- warehouse/material persisted snapshots;
- repair status/finalization preflight;
- UI pages showing totals;
- backup/integrity coverage if new persisted data appears.

Rule: historical cost remains based on persisted operation snapshots, not today's price.

## 11. Add or change persistent data on microSD

Open the writer and authoritative reader first.

Then inspect all applicable layers:

```text
CM_FlatJsonObjectValidator.h
<domain>PersistenceIntegrityAudit.*
CM_BackupBusinessDataIntegrityAudit.*
CM_BackupExportWeb.*
CM_RemoteBackupWeb.*
restore plan / rollback / apply code
Tests/Web/check_final_acceptance_contracts.js
```

Before adding a new path, answer:

1. What is the exact `/data/...` path?
2. Is it append-only, replaceable, snapshot, journal or state?
3. What is its atomic/pending strategy?
4. How are corrupted rows detected?
5. How are old rows parsed?
6. What references must be cross-validated?
7. Is it included in deep integrity audit?
8. Is it included in backup manifest/restore plan?
9. Does rollback cover it?
10. What happens after reboot mid-write?

If backup/restore does not know the file exists, the persistence feature is incomplete.

## 12. Change backup/export/remote backup/restore

Open first:

```text
CM_BackupActivityGuard.*
CM_BackupExportWeb.*
CM_BackupBusinessDataIntegrityAudit.*
CM_RemoteBackupSettings.*
CM_RemoteBackupTransfer.*
CM_RemoteBackupWeb.*
CM_WebRecoveryFtpServer.*
```

And domain integrity audits:

```text
CM_WorkshopPersistenceIntegrityAudit.*
CM_MaterialPersistenceIntegrityAudit.*
CM_WarehousePersistenceIntegrityAudit.*
CM_PersistentIdIntegrityAudit.*
CM_ConductorSettingsIntegrityAudit.*
CM_WindingPersistenceIntegrityAudit.*
CM_WindingSessionPersistenceIntegrityAudit.*
```

Safety semantics to preserve:

- heavy scans only when machine state is provably safe;
- explicit operator restore/apply;
- exact batch identity;
- staged source validation;
- rollback snapshot before replacement;
- persisted apply evidence;
- reboot → STALE, never resume;
- explicit cleanup;
- no automatic production-data deletion.

Backup/restore changes deserve a narrow contract audit before optimization.

## 13. Change network, Wi-Fi, AP/STA, mDNS or diagnostics

Open first:

```text
CM_NetworkProfileStore.*
CM_NetworkManager.*
CM_NetworkWeb.*
main.cpp
CM_StaticSiteServer.*
```

For read-only system/storage diagnostics also inspect:

```text
CM_StorageDiagnosticsWeb.*
firmware/esp32/web/shared/settings-system-diagnostics.js
```

Network behavior must remain bounded/non-blocking enough not to interfere with core service operation.

Do not treat `coil.local` as the only operational access path; IP fallback remains required.

## 14. Change incoming `/web` recovery FTP

Open first:

```text
CM_WebRecoveryFtpServer.*
CM_StaticSiteServer.*
main.cpp
```

Contract:

- recovery FTP is for `/web` only;
- it must not expose `/data`;
- staging/backup behavior must remain safe;
- it is distinct from outbound remote backup transfer.

## 15. Add an ESP32 hardware module (sensor, display, peripheral)

Open first:

```text
firmware/esp32/src/main.cpp
docs/PROJECT_HANDOFF/02_ARCHITECTURE_AND_HARDWARE.md
```

Then use `03_ADD_MODULE_PLAYBOOK.md`.

Before choosing pins, compare against current reserved ESP32 pins:

```text
GPIO16/17 UART2
GPIO5/18/19/23 microSD SPI
GPIO21/22 RTC I2C
```

Define power, logic voltage, bus ownership, startup failure behavior and diagnostics before integration.

## 16. Add an Arduino hardware module

Open first:

```text
Arduino/Config/CM_Pins.h
firmware/arduino/src/main.cpp
Core/
Arduino/
platformio.ini
```

Constraints:

- Uno SRAM/flash budget matters;
- do not allocate large dynamic strings/buffers casually;
- do not steal physical START/SSR/Hall/UART pins;
- hardware adapter should not bypass the state machine.

## 17. Change motor import format

Open first:

```text
docs/MOTOR_IMPORT_FORMAT.md
CM_RepairRegistryWeb.*
CM_MotorSimilarityWeb.*
firmware/esp32/web/desktop/motor-import.html
firmware/esp32/web/mobile/motor-import.html
Tests/Web/check_web_assets.js
```

Keep:

- preview before write;
- strict documented field names;
- provenance rules;
- package duplicate detection;
- one-record server append semantics;
- persistence after reboot.

## 18. Change storage-capacity behavior

Read-only diagnostics live in:

```text
CM_StorageDiagnosticsWeb.*
settings-system-diagnostics.js
```

Release policy:

```text
automatic_cleanup_allowed = false
```

Do not turn a low-space condition into automatic deletion of production records.

## 19. Change tests or CI only

Open:

```text
.github/workflows/*.yml
Tests/Protocol/
Tests/CMP1Text/
Tests/Web/
```

A test/docs-only commit does not automatically invalidate closed hardware gates or require a firmware reflash.

Do not report a workflow as green until the actual run has completed successfully.

## 20. Search order when the router is not enough

Use this order instead of repository-wide guessing:

```text
1. Search exact API route / class / data path / protocol token.
2. Open its owner (`main.cpp`, `CM_StaticSiteServer`, or parent service).
3. Open matching `*Web`, store/ledger, validator, integrity audit.
4. Open mobile + desktop consumers.
5. Open contract tests.
6. Only then broaden the search by class prefix.
```

Avoid using old handoff text to infer current implementation when the current source is available.
