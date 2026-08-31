# Checkpoint 27 — Post-release dead-code / repository cleanup audit

Date: 2026-08-31
Branch: `arduino-ru-lcd-experiment`

## Baseline

Audit started from:

```text
41c6e7056c6c1ce8bc2481a58c8755ac5675245b
```

Production `cmp-protocol-v1` was not modified by this cleanup pass.

## Proven cleanup

Removed 18 historical one-shot `*-patch.yml` workflows from `.github/workflows/` in one commit:

```text
eadc035bcf376b7be46278f89df1adf121cff18f
chore(ci): remove archived one-shot patch workflows
```

Seventeen removed workflows were current-content archived/no-op helpers (`workflow_dispatch` + `echo` only).

The remaining removed workflow:

```text
.github/workflows/material-request-transaction-ref-audit-patch.yml
```

was an obsolete one-shot mutation helper with `contents: write` and a guarded direct push to `cmp-protocol-v1`. It is no longer appropriate after the first stable release and permanent regression wiring.

Exact verification for the deletion:

```text
CMP Protocol Tests #4692
run 33363104766
head eadc035bcf376b7be46278f89df1adf121cff18f
completed / success
```

No firmware source was changed by this removal.

## Regression classification / wiring

Do not classify a `Tests/Web/check_*` file as orphan merely because it is not a top-level workflow step. Several tests are intentionally invoked by another mandatory regression.

Confirmed existing indirect coverage:

```text
check_motor_details_ui.js
  -> check_dashboard_job_history.js
  -> check_client_crm_ui.js
       -> check_cash_ui.js

check_material_request_warehouse_coordinator.js
  -> check_run_wire_issue_transaction.js
```

Therefore `check_client_crm_ui.js`, `check_cash_ui.js` and `check_run_wire_issue_transaction.js` are KEEP, not orphan.

New permanent regression wiring added during this audit:

```text
6851092bd307cce0cf733f661eed481818e4ac4a
test(web): wire live CRM and CRUD regressions
```

The initial `6851092...` wiring intentionally tested both CRM and CRUD and passed CMP #4694, but follow-up dependency review proved CRM/Cash were already invoked by `check_motor_details_ui.js`. The duplicate CRM call was then removed; only the genuinely unwired CRUD separation test remains attached to the shared app-shell mandatory step.

Current corrected wiring:

```text
0ab2ce70cf5aa5a1936a46cfa2966c8292016b44
test(web): avoid duplicate CRM regression execution

check_shared_app_shell_contracts.js
  -> check_crud_page_separation.js
```

Motor-role/edit regressions were also proven live and attached to the already mandatory motor-details step:

```text
41e1db3d2672ecd78176515ff17340536df15176
test(motors): wire role and edit regressions

check_motor_details_ui.js
  -> check_motor_edit_ui.js
  -> check_linked_job_winding_role.js
  -> check_winding_job_role_ui.js
```

Exact verification:

```text
CMP Protocol Tests #4696
run 33363561029
head 41e1db3d2672ecd78176515ff17340536df15176
completed / success
```

Spool/material bridge regressions were proven live and attached to the already mandatory Material Request warehouse coordinator step:

```text
99134f607f7da86cd74b4d257b0211674b8c02d1
test(materials): wire spool bridge regressions

check_material_request_warehouse_coordinator.js
  -> check_spool_material_bridge_store.js
  -> check_spool_material_bridge_web.js
  -> check_run_wire_issue_transaction.js
```

Exact verification:

```text
CMP Protocol Tests #4697
run 33363646866
head 99134f607f7da86cd74b4d257b0211674b8c02d1
completed / success
```

## C++ review status

No new C++ production file/function has been deleted in this pass because no concrete dead owner is proven yet.

Important KEEP example:

```text
CM_ClientRevisionWeb
```

It is not instantiated directly in `main.cpp`, but is a member of `RepairRegistryWeb`, which is the production bootstrap owner. Therefore absence from `main.cpp` alone is not dead-code proof.

Likewise split implementation files that implement methods of a live class (for example lookup/store `.cpp` files) remain KEEP until an absent call-site or duplicate authoritative owner is proven.

## Keep / do not delete

`Shared/Protocol/` remains KEEP. It is not the production UART owner, but it is still compiled by `Tests/Protocol/CMakeLists.txt` for host regression tests.

Production/runtime owner directories remain KEEP:

```text
Core/
Arduino/
firmware/arduino/
firmware/esp32/src/
firmware/esp32/web/
Shared/CMP1Text/
lib/CM_Keypad/
lib/CM_LcdCompat/
scripts/platformio_build_id.py
```

Historical `docs/PROJECT_HANDOFF/` checkpoints remain historical evidence and are not cleanup candidates merely because numbering overlaps.

## Next cleanup pass

Continue classifying remaining non-top-level `Tests/Web/check_*` files as:

```text
LIVE INDIRECT      -> KEEP; already invoked by another mandatory test
LIVE BUT UNWIRED   -> connect to the closest permanent regression owner
STALE              -> remove only after current-source contract proof
DUPLICATE          -> merge/remove only when equivalent permanent coverage is proven
KEEP               -> directly wired or independently required
```

Then continue C++ private-helper/file review only on concrete candidates. Do not remove production code solely because it is not referenced directly from `main.cpp`.
