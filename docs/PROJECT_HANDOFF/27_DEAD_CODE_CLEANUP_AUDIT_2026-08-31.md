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

## Exact verification

```text
CMP Protocol Tests #4692
run 33363104766
head eadc035bcf376b7be46278f89df1adf121cff18f
completed / success
```

Do not infer ESP32/Arduino build status from this workflow-only cleanup; no firmware source was changed.

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

Do not bulk-delete `Tests/Web/check_*` files.

The current audit already found examples of tests that are not visibly wired into `cmp-protocol-tests.yml` but protect live production contracts, including:

```text
check_cash_ui.js
check_client_crm_ui.js
check_crud_page_separation.js
```

Classify remaining unwired tests as:

```text
LIVE BUT UNWIRED  -> connect to permanent CI
STALE              -> remove only after current-source contract proof
DUPLICATE          -> merge/remove only when equivalent permanent coverage is proven
KEEP               -> already permanently wired or independently invoked
```

Next step: complete this classification before any further test deletion, then continue C++ function/file duplicate/dead-code review on only concrete candidates.
