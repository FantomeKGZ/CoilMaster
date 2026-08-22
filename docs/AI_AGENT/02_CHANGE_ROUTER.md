# CoilMaster — change router for AI agents

Use this document when the task is known but the implementation location is not.

For every route below, fetch the listed files from `cmp-protocol-v1` immediately before editing. The list is a starting set, not permission to ignore current call sites.

Do not use historical handoff `next` sections to choose work. Current active work is selected by `docs/PROJECT_HANDOFF/63_FULL_CODE_AUDIT_2026-08-22.md`, `docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md`, a concrete current failure or the user's explicit request.

## 1. Physical START, SSR, Hall, keypad, LCD or buzzer

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

Safety impact:

- physical START remains physical;
- SSR authority stays on Arduino;
- no automatic START between repeat runs;
- lost communication/errors fail safe;
- reboot never auto-resumes movement.

Verification: Arduino Uno Build + relevant host tests + targeted hardware test when physical behavior changed.

## 2. UART/CMP1 frame, CRC, ACK/NACK, cancel or retry behavior

Open first:

```text
Arduino/CM_UartEventTransport.h/.cpp
firmware/esp32/src/CM_UartEventReceiver.h/.cpp
Shared/CMP1Text/CM_Cmp1Crc.h
firmware/arduino/src/main.cpp
firmware/esp32/src/main.cpp
Tests/Protocol/
Tests/CMP1Text/
docs/PROJECT_HANDOFF/03_PROTOCOL_AND_WINDING_FLOW.md
```

Required questions:

- Is wire compatibility preserved?
- Are field count/length/range/CRC checks strict?
- Are retries bounded?
- Can any ACK/cancel state accidentally imply physical START or completed run?
- Is already-clear cancellation idempotent where intended?

Current JOB cancel/recovery is implemented and closed: `ALREADY_CLEAR`, safe `ALL_CLEAR`, physical `D -> * -> # -> D` fallback and no auto-start/run-complete semantics. Do not reopen it without a concrete regression.

Verification: CMP Protocol Tests + Arduino Uno Build + ESP32 Build + targeted hardware UART regression when wire behavior changes.

Do not use `Shared/Protocol/` as a production shortcut; it is the older binary host-test protocol.

## 3. ESP32 HTTP API endpoint

Open `firmware/esp32/src/main.cpp`, `CM_StaticSiteServer.*`, then the owning `*Web.*` and domain store/service.

Check method/path/status/error semantics, strict validation, bounded request/response sizes, fail-closed behavior, mutation semantics, UI consumers and Web audit coverage.

Verification: ESP32 Build + affected `Tests/Web/` audits; hardware only when device behavior requires proof.

## 4. Web page / operator UI

Open:

```text
firmware/esp32/web/desktop/
firmware/esp32/web/mobile/
firmware/esp32/web/shared/
Tests/Web/check_web_assets.js
Tests/Web/check_release_contracts.js
Tests/Web/check_final_acceptance_contracts.js
```

Maintain desktop/mobile parity where functionality is operator-facing. Shared dynamic markup must be tested where it is actually generated rather than assuming it exists statically in HTML.

Never make UI state imply that a physical action happened before confirmed hardware evidence.

## 5. Clients, motors or repairs

Open `CM_RepairRegistry*`, `CM_RepairRegistryWeb*`, lookup/similarity modules and `/data/workshop/` integrity/backup coverage.

For motor import also inspect `docs/MOTOR_IMPORT_FORMAT.md` and desktop/mobile import pages.

Old persisted records must remain intentionally readable or have an explicit migration plan.

## 6. Linked winding job creation

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

Preserve immutable job identity/snapshot, current linkage rules, and the boundary that UART acceptance never becomes physical START.

If exact spool selection is present in the linked-job path, preserve it exactly. Do not infer that KG_FIRST optional-spool writeoff automatically removes linked-job selection requirements elsewhere; inspect current code before changing that contract.

## 7. RUN_STARTED / RUN_COMPLETED journaling or winding history

Open `CM_UartEventReceiver.*`, `CM_WindingJournal*`, `CM_WindingJournalQuery*`, `CM_WindingPersistenceIntegrityAudit.*` and `CM_WindingJournalTransitionAudit.*`.

Important:

- authoritative full validation uses `WindingJournalQuery::validateAll()`;
- transition audit is separate;
- do not reintroduce pagination as authoritative full validation;
- `RUN_COMPLETED` never writes off material automatically.

## 8. Warehouse, wire spool, pricing, movements or manual writeoff

Open first by prefix:

```text
firmware/esp32/src/CM_Warehouse*.h/.cpp
```

For writeoff also inspect:

```text
CM_WarehouseWriteOffWeb.*
CM_JobSpoolSelectionStore.*
CM_WindingJournalQuery*
Tests/Web/check_kg_first_material_contracts.js
Tests/Web/check_writeoff_fault_contracts.js
```

Current provenance rules:

```text
source_session_id + source_run_id  mandatory for new run-linked consumption
spool_id                           optional only in approved KG_FIRST unallocated/manual path
```

When a spool is used, exact spool identity/provenance and stock decrement must remain exact. Legacy exact-spool records remain supported.

Rules:

- writeoff remains explicit/manual;
- completed source run must be proven;
- duplicate exact-run writeoff remains rejected;
- `RUN_COMPLETED` alone never mutates warehouse/material stock;
- historical price/material snapshots are not recomputed from current values;
- fault paths remain fail-closed before partial mutation.

Also inspect warehouse persistence/movement integrity and backup coverage.

## 9. Conductor calculator / Al-Cu conversion settings / standard wire alternatives

Open first:

```text
CM_ConductorCalculator.*
CM_ConductorCalculatorWeb.*
CM_ConductorSettingsWeb.*
CM_ConductorSettingsStore.cpp
CM_WarehouseStore.*
CM_StandardWireCatalogue.*
firmware/esp32/web/desktop/calculator.html
firmware/esp32/web/mobile/calculator.html
Tests/Web/check_calculator_source_wire_input.js
```

Authoritative production settings are persisted by `WarehouseStore::loadConversionSettings()` / `setConversionSettings()` at:

```text
/data/settings/conductor.json
```

Do not make the parallel legacy `CM_ConductorSettings.*` persistence class a second production owner. It is a post-audit cleanup candidate pending dependency proof.

Current calculator accepts 1..5 source wires from one semicolon-separated UI field and maps each entered diameter to one source component. Warehouse recommendations and read-only standard-reference recommendations are separate. The standard catalogue must never create stock/spools or imply that a purchase already occurred.

## 10. Auxiliary materials / material usage

Open `firmware/esp32/src/CM_Material*.h/.cpp`, then costing/pricing integration and `CM_MaterialPersistenceIntegrityAudit.*`.

Validate material identity, repair identity, quantity/stock rules, currency policy, persisted cost snapshot and pending/recovery semantics.

## 11. Costing, finalization or pricing

Open `CM_RepairCosting*` and `CM_RepairPricing*`, then warehouse/material persisted snapshots, finalization preflight and both UIs.

Historical cost must remain based on persisted operation snapshots, not current prices.

## 12. Persistent data on microSD

Open writer + authoritative reader, then applicable validator, domain integrity audit, backup/export, restore plan/rollback/apply and final-acceptance tests.

Before adding a path answer: exact `/data/...` location, atomic/pending strategy, corruption detection, old-record compatibility, cross-reference validation, deep-audit inclusion, backup/restore inclusion and reboot-mid-write behavior.

A production data file unknown to backup/integrity logic is incomplete integration.

## 13. Backup/export/remote backup/restore

Open:

```text
CM_BackupActivityGuard.*
CM_BackupExportWeb.*
CM_BackupBusinessDataIntegrityAudit.*
CM_RemoteBackupSettings.*
CM_RemoteBackupTransfer.*
CM_RemoteBackupWeb.*
CM_WebRecoveryFtpServer.*
```

Plus all domain persistence integrity audits.

Preserve safe-idle gating, strict source validation, rollback snapshot, explicit operator APPLY, persisted stale evidence, reboot -> STALE/no resume and no automatic production-data deletion.

`WindingSessionPersistenceIntegrityAudit` owns authoritative read-only session preflight; do not reintroduce a duplicate backup manifest full scan.

## 14. Network, Wi-Fi, AP/STA, mDNS or diagnostics

Open `CM_NetworkProfileStore.*`, `CM_NetworkManager.*`, `CM_NetworkWeb.*`, `main.cpp`, `CM_StaticSiteServer.*` and relevant diagnostics modules.

Network behavior must remain bounded/non-blocking enough not to interfere with core service operation. Keep IP fallback; do not assume `coil.local` is always available. JSON responses that contain SSID/operator-controlled strings must use complete JSON escaping, including control bytes.

## 15. Incoming `/web` recovery FTP

Open `CM_WebRecoveryFtpServer.*`, `CM_StaticSiteServer.*`, `main.cpp`.

Recovery FTP is `/web` only, not `/data`, and remains distinct from outbound remote backup.

## 16. ESP32 hardware module

Open `firmware/esp32/src/main.cpp`, hardware docs and `03_ADD_MODULE_PLAYBOOK.md`.

Check reserved pins before assignment:

```text
GPIO16/17 UART2
GPIO5/18/19/23 microSD SPI
GPIO21/22 RTC I2C
```

Define power, logic voltage, bus ownership, startup failure behavior and diagnostics.

## 17. Arduino hardware module

Open `Arduino/Config/CM_Pins.h`, `firmware/arduino/src/main.cpp`, `Core/`, `Arduino/`, `platformio.ini`.

Uno SRAM/Flash budgets matter. Do not bypass the state machine or steal START/SSR/Hall/UART pins.

## 18. Motor import format

Open `docs/MOTOR_IMPORT_FORMAT.md`, registry/similarity Web modules, both import pages and Web audits.

Keep preview-before-write, strict documented fields, provenance, duplicate detection, one-record append semantics and reboot persistence.

## 19. Storage-capacity behavior

Read-only diagnostics live in `CM_StorageDiagnosticsWeb.*` and settings diagnostics JS.

```text
automatic_cleanup_allowed = false
```

Low-space conditions must not trigger automatic deletion of production records.

## 20. Tests or CI only

Open `.github/workflows/*.yml`, `Tests/Protocol/`, `Tests/CMP1Text/`, `Tests/Web/`.

Test/docs-only commits do not automatically invalidate closed hardware gates. Do not report a workflow green until its actual run completed successfully.

The CMP Protocol workflow intentionally continues later Web/safety audits after an earlier audit failure so one run exposes all failing contracts; any failed audit must still fail the job.

Shared production code (`Shared/**`) must trigger all applicable Arduino/ESP32/protocol gates. `Tests/Web/check_ci_trigger_contracts.js` protects the critical trigger coverage.

## 21. Post-audit cleanup / dead code / duplicate files

Only start after sections A..E and final cross-layer recheck are complete.

For each candidate prove:

```text
C++ includes/call sites
PlatformIO build_src_filter ownership
workflow/test references
runtime HTTP/static injection
web imports/script loaders
runtime microSD path compatibility
AI/docs routing/history requirements
```

Then classify `DELETE`, `MERGE`, `KEEP` or `REVIEW`. Never remove production data automatically. Known candidates may be documented during audit, but deletion waits for the cleanup phase.

## 22. Search order when the router is not enough

```text
1. Search exact API route / class / data path / protocol token.
2. Open its runtime owner.
3. Open matching Web/store/validator/integrity audit.
4. Open mobile + desktop consumers.
5. Open contract tests.
6. Only then broaden by class prefix.
```

Avoid using old handoff text to infer current implementation when current source is available.
