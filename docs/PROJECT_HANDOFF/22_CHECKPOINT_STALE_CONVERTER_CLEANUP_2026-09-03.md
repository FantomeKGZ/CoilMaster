# Checkpoint 22 — stale conductor converter cleanup

Date: 2026-09-03

## Scope

Closed a real user-facing stale Web defect on the active development branch `cmp-protocol-v1`.

Current branch policy:

- `cmp-protocol-v1` — only active development/source branch;
- `main` — stable/ready state only;
- `arduino-ru-lcd-experiment` — retired and must not be used as a development source.

## Finding

The current authoritative conductor calculator is implemented in:

```text
firmware/esp32/web/desktop/calculator.html
firmware/esp32/web/mobile/calculator.html
```

It uses persisted server-side conversion settings and supports 1–5 source wire components with explicit strand counts.

Two older compatibility pages still existed:

```text
firmware/esp32/web/desktop/conductor-converter.html
firmware/esp32/web/mobile/conductor-converter.html
```

Those pages still exposed user-entered `allowed_deviation_permille` and `max_target_strands` request parameters. The current backend no longer takes those values from the request; it loads authoritative persisted settings. The old pages also rendered the pre-multi-component response shape (`diameter_hundredths_mm`, `available_g` at top level) while the current API returns component arrays.

Therefore the old pages were stale and could present misleading or broken results even though the authoritative calculator was correct.

## Fix

The stale URLs remain present as compatibility aliases but now redirect to the authoritative calculator for the same UI mode.

Commits:

```text
2523b5213f7519b0debd305273c67de361bf7bdd  desktop compatibility redirect
df7140cf32a1a05e34a404709bc01df3861b33e2  mobile compatibility redirect
7164e65b792a2dd611833b0992d21ede50183c6d  regression contract for aliases
65278d750ae39da7bc18369cc7fd03fad4739a1b  desktop shared-shell compatibility
7c62552c555380b6c38a8ea82eded48d259684b4  mobile shared-shell compatibility
```

The first alias implementation intentionally exposed an existing shared-shell requirement: every desktop/mobile HTML route must retain a usable `<main>` host. The compatibility aliases were updated accordingly without restoring the stale calculator UI.

## Regression protection

`Tests/Web/check_calculator_source_wire_input.js` now additionally enforces that:

- both legacy `conductor-converter.html` routes redirect to the authoritative mode-specific `calculator.html`;
- the stale user-controlled conversion settings do not return;
- the stale legacy response rendering does not return.

The existing shared-shell contract continues to require valid variant HTML and route parity.

## Exact verification

Current code HEAD:

```text
7c62552c555380b6c38a8ea82eded48d259684b4
```

Verified on that exact HEAD:

```text
CMP Protocol Tests #4834
run 33729342928
completed/success

Reference Legacy Import Check #110
run 33729342866
completed/success

ESP32 Build #1874
run 33729343011
completed/success
```

Intermediate CMP failures `#4830–#4833` belong to the discovery/fix sequence and are not final GREEN evidence.

## Documentation alignment in same work stream

The project entry documentation was also aligned with the current branch policy before this stale-page fix:

```text
49442512b8c7f927a295daa5401b941f5e7ee4f9  00_READ_FIRST.md
c9b926ebc9281b8e09964ec0d2d147cbe9f34c6c  06_ACTIVE_WORK_AND_NEXT_STEPS.md
a947aae8e2f962436bae7eed28b5ab8c58157b6f  01_CURRENT_STATE.md
```

Associated CMP runs `#4827`, `#4828`, `#4829` completed/success.

## Safety invariants unchanged

No physical-control, winding, storage mutation, writeoff or SSR authority changed.

Still enforced:

- physical START only;
- no auto-resume after reboot;
- Arduino remains sole SSR owner;
- ESP32/Web do not directly control SSR;
- `RUN_COMPLETED` alone does not write off wire;
- manual wire writeoff retains exact spool/session/run provenance.

## Next step

Continue the feature-completeness audit only from a newly proven runtime/user-facing gap. Do not reopen already-closed Uno micro-optimization, orphan-test or stale Statistics items without new evidence.
