# NEXT CHAT TRANSFER — 2026-08-30 — LATE

Repository: `FantomeKGZ/CoilMaster`  
Production/source-of-truth: `cmp-protocol-v1`  
Working branch: `arduino-ru-lcd-experiment`

## Branch policy

- Do not use `main` as source.
- Do not modify `cmp-protocol-v1` without an explicit user request.
- Continue only on `arduino-ru-lcd-experiment`.
- Before modifying an existing file, fetch the current branch content and use its current blob SHA.

Production remains unchanged:

```text
cmp-protocol-v1 = 28c7917a906bc9b15736369e8986d0e0c354ab8c
```

## Latest confirmed GREEN code/test checkpoint

```text
d84402251552522f60c2494da7dd7b19bb6af35a
CMP Protocol Tests #4630   run 33320670860 / SUCCESS
Arduino RU LCD Build #241 run 33320670903 / SUCCESS
```

The later documentation commits must not be called GREEN unless their own exact runs are checked.

## CI failure series #4611–#4619

This series was not a runtime/protocol regression. The first failure was caused by stale Web text-contract expectations after Russian localization of system diagnostics.

Only the diagnostics UI text assertions were stale; protocol tests, safety/recovery audits and ESP32/Arduino builds remained healthy. The expectations were aligned with the current Russian wording in:

- `Tests/Web/check_web_assets.js`
- `Tests/Web/check_ndjson_growth_diagnostics.js`

Recovery was confirmed by CMP #4624 and then again by CMP #4630.

## Completed bounded create-only integrity sweep

Fast double click/tap could previously send duplicate persistent create mutations on several dedicated create pages.

The following persistent create flows are now single-flight in desktop/mobile as applicable:

- client — mobile parity fixed; desktop already guarded;
- motor — similarity-check and create are separately guarded on desktop/mobile;
- repair — desktop/mobile guarded;
- spool — desktop/mobile guarded;
- material — desktop/mobile guarded.

Common behavior now includes:

- a state guard rejecting a second submit while the first mutation is active;
- disabled create/save control during mutation;
- required returned entity ID (`client_id`, `motor_id`, `repair_id`, `spool_id`, `material_id`) before success;
- control restoration on error where applicable.

Regression coverage:

- `Tests/Web/check_client_crm_ui.js`
- `Tests/Web/check_crud_page_separation.js`
- motor/Wi-Fi related existing Web contracts remain GREEN in the same CMP suite.

Detailed checkpoint:

`docs/PROJECT_HANDOFF/09_CHECKPOINT_2026-08-30_CI_RECOVERY_AND_CREATE_SINGLE_FLIGHT.md`

## Do not continue these sweeps

- Repeated-scan optimization is closed as NO-CHANGE unless a measured bottleneck appears.
- Create-only duplicate-submit audit is complete for current dedicated persistent CRUD create pages.
- Do not broaden into generic edit/list cleanup without a reproducible mutation defect.
- Do not grow Uno features speculatively; RU build flash headroom is tight.

## Safety invariants

Do not change:

- no automatic physical START/repeat START;
- no auto-resume after reboot;
- Arduino is sole SSR owner;
- ESP32/Web never controls SSR directly;
- `RUN_COMPLETED` is evidence only;
- RUN_WIRE deduction is explicit/manual;
- exact `spool_id + source_session_id + source_run_id` is mandatory;
- restore/recovery stays fail-closed/operator-controlled;
- mutation-time TOCTOU/recovery rereads stay authoritative;
- confirmed history remains append-only;
- no automatic production truncation/rotation/deletion.

## Immediate next engineering gate

Without a newly reproduced software defect, the next required gate is physical Arduino + ESP32 E2E on the real CoilMaster.

Minimum E2E checklist:

1. ESP32 command -> Arduino ACK.
2. Keypad responsiveness before and after Hall mode.
3. Normal RU LCD screens before Hall; RU Hall screens during test; normal CGRAM restoration after exit.
4. Physical START remains Arduino-only; no Web/ESP32 SSR control path.
5. Hall 15-second run; apply and reject paths.
6. SSR fail-safe behavior.
7. RUN_STARTED/RUN_COMPLETED evidence without automatic wire deduction.
8. Manual exact RUN_WIRE writeoff with `spool_id + source_session_id + source_run_id`.
9. Reboot/recovery fail-closed and no auto-resume.

If hardware E2E is not being performed, wait for a concrete reproducible repo/runtime defect rather than starting another speculative broad audit.
