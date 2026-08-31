# Legacy integrity audit cleanup — 2026-08-31

Branch: `arduino-ru-lcd-experiment`.
Production `cmp-protocol-v1` was not modified.

## RUN_WIRE backup integrity restored

`CM_BackupExportWeb::snapshotStabilityReason()` now directly executes `RunWireAccountingIntegrityAudit::check(storage)` and fails closed with `run_wire_accounting_unstable_or_invalid`.

Implementation:
- `b46c2912ba3d6f451c5de0cac28de429fe979256`

Exact verification:
- CMP #4706 / run `33365896377` / SUCCESS
- ESP32 #1836 / run `33365896362` / SUCCESS
- Arduino RU LCD #267 / run `33365896373` / SUCCESS
- regression head `0e218c3b9c20366549d081272752a23457bfdf70`: CMP #4707 / SUCCESS

No RUN_WIRE mutation/writeoff/recovery semantics changed. Exact manual provenance remains `spool_id + source_session_id + source_run_id`.

## Broad standalone integrity owner refactor

Standalone `MaterialPersistenceIntegrityAudit::check(storage)` no longer depends on the legacy `WorkshopPersistenceIntegrityAudit` wrapper or duplicate `RepairPricingIntegrityAudit` owner.

It now directly retains the broad fail-closed contract through authoritative owners:
- `BackupBusinessDataIntegrityAudit::check(storage)`;
- `WarehousePersistenceIntegrityAudit::check(storage)`;
- `PersistentIdIntegrityAudit::check(storage)`;
- `WindingSessionPersistenceIntegrityAudit::check(storage)`;
- `RunWireAccountingIntegrityAudit::check(storage)`;
- `WindingPersistenceIntegrityAudit::check(storage)`;
- explicit `/data/workshop` directory-shape validation.

Refactor:
- `669b3619490f97bd843d88bdf814c6c782e24998`

Build verification for that code:
- ESP32 #1837 / run `33366286947`: build job SUCCESS
- Arduino RU LCD #268 / run `33366286983`: compare-builds job SUCCESS

Regression owner update:
- `93b02e5f5effcfacc9c3b241a95a757bf7ebbc99`
- CMP #4709 / run `33366320032` / SUCCESS

CMP #4708 was an intermediate stale-test failure because the old assertion still required the retired wrapper.

## Removed C++ legacy modules

Deleted from the current tree:
- `CM_WorkshopPersistenceIntegrityAudit.cpp`
- `CM_WorkshopPersistenceIntegrityAudit.h`
- `CM_RepairPricingIntegrityAudit.cpp`
- `CM_RepairPricingIntegrityAudit.h`

Last code-changing deletion SHA:
- `e454a0228d2bcfddee90a2ef50af903232441ce0`

Exact build evidence on that SHA:
- ESP32 #1841 / run `33366606226` / SUCCESS
- Arduino RU LCD #272 / run `33366606240` / SUCCESS

CMP #4718 / run `33366606237` was an intermediate stale-regression failure only: two JS audits still opened `CM_WorkshopPersistenceIntegrityAudit.cpp` by path. Host CTest and the remaining regressions passed.

The stale tests were moved to the new direct authoritative owners:
- `44ba976a41c3232080ff155be55a00475a09e852` — RUN_WIRE transaction audit
- `0621350c27e864a8e75491bdc8c1ff172451666a` — workshop winding single-pass audit

Final exact CMP evidence:
- CMP #4725 / run `33366984052` / HEAD `0621350c27e864a8e75491bdc8c1ff172451666a` / SUCCESS

## Cleanup tooling note

Several temporary `.cleanup-delete-placeholder*` files were created by unsuccessful staging attempts while trying to make an atomic tree deletion. They were immediately removed; none exists in the current tree. No force update was used.

## Next cleanup step

Continue the experiment-only dead-code audit with remaining C++ modules and test/filesystem candidates. Delete only after proving absent/duplicate ownership and require exact CMP plus applicable ESP32/Arduino build evidence for every code-changing removal.
