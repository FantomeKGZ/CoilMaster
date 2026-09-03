# Checkpoint 28 — repair creation legacy handoff parity

Date: 2026-09-03

## Scope

Closed a concrete desktop/mobile runtime parity gap in the repair-creation handoff on active branch `cmp-protocol-v1`.

## Proven defect

The current repair-creation contract uses the dedicated pages:

```text
/desktop/repair-new.html
/mobile/repair-new.html
```

Legacy links into `repairs.html?client_id=...` and/or `?motor_id=...` are creation handoffs and must forward those identifiers into the matching `repair-new.html` page.

Desktop already behaved this way. Mobile still interpreted `client_id` as a hidden catalog filter and did not preserve the same creation handoff semantics.

## Runtime fix

Updated:

```text
firmware/esp32/web/mobile/repairs.html
```

Mobile now:

- accepts `client_id` and/or `motor_id` from the URL;
- redirects to `/mobile/repair-new.html` with the supplied identifiers preserved;
- no longer reinterprets `client_id` as an implicit repair-list filter;
- keeps normal repair catalog pagination/filtering unchanged when no creation handoff is present.

Runtime commit:

```text
229625e5ce9111a25e067e70aa95f2f5363c9797
fix(web): align mobile repair creation handoff
```

## Regression protection

Extended:

```text
Tests/Web/check_repair_new_ui.js
```

The regression now protects the semantic contract on both desktop and mobile:

- `client_id` handoff into repair creation;
- `motor_id` handoff into repair creation;
- matching desktop/mobile `repair-new.html` destination;
- mobile must not reintroduce `client_id` as a hidden catalog filter.

Intermediate regression commit:

```text
fcdfb7a7144d31f2172c7d0ce48523b94cfff300
```

That version over-constrained the desktop implementation by requiring one specific redirect construction (`new URL(...)`) rather than testing the handoff semantics.

Exact CI result:

```text
CMP Protocol Tests #4857
run 33733174978
completed/failure
```

The failure was isolated to `Audit web JavaScript and navigation` and was exactly:

```text
Error: desktop repairs legacy creation handoff missing: new URL('/desktop/repair-new.html',location.origin)
```

The underlying desktop runtime handoff remained valid; the regression itself was too implementation-specific.

Final regression commit:

```text
a531a063b151fc3fcd356c11bff9d2deeee5c2ad
test(web): check repair handoff semantics
```

The test was corrected to check the actual contract instead of requiring a particular local implementation style.

## Exact CI evidence

Runtime commit `229625e5...`:

```text
CMP Protocol Tests #4856
run 33733146747
completed/success

Reference Legacy Import Check #119
run 33733146680
completed/success

ESP32 Build #1883
run 33733146716
completed/success
```

Final regression HEAD `a531a063...`:

```text
CMP Protocol Tests #4858
run 33733234029
completed/success
```

## Safety invariants unchanged

No physical START, SSR, UART, warehouse, costing, wire writeoff or finalization mutation semantics changed.

Still enforced:

- physical START only;
- no auto-resume after reboot;
- Arduino owns SSR;
- ESP32/Web do not directly control SSR;
- `RUN_COMPLETED` alone does not write off wire;
- manual wire writeoff keeps exact `spool_id`, `source_session_id`, `source_run_id` provenance.

## Next step

Continue only from another proven user-facing/runtime gap. Repair creation and the legacy client/motor handoff are now aligned across desktop/mobile and protected by exact GREEN regression coverage.