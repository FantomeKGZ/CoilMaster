# Checkpoint 31 — Pricing audit HTML escaping — 2026-09-03

Branch: `cmp-protocol-v1`

## Confirmed defect

Desktop/mobile `pricing-audit.html` rendered two server-derived strings directly inside `current.innerHTML`:

- `pricing_status`;
- fallback/raw `pricing_updated_at` when the timestamp could not be parsed by the browser.

The financial model and source-of-truth were not changed. The page still reads the existing authoritative costing and append-only pricing-history APIs.

## Fix

Affected files:

- `firmware/esp32/web/desktop/pricing-audit.html`;
- `firmware/esp32/web/mobile/pricing-audit.html`;
- `Tests/Web/check_settings_hub_parity.js`.

Both pages now HTML-escape the two dynamic values before interpolation into `innerHTML`:

```text
esc(c.pricing_status||'—')
esc(dateLabel(c.pricing_updated_at))
```

Commits:

```text
16ff2dd76c5b549e18d0edab6784716837662907  fix(web): escape desktop pricing audit status
755a0b7837fe8d4c1479bf5e7705255ed2ded0a1  fix(web): escape mobile pricing audit status
93876783690fa0fb4c47539c2af240ef77962cc5  test(web): protect pricing audit escaping
```

## Exact verification

```text
CMP #4877
run 33735336317
head 93876783690fa0fb4c47539c2af240ef77962cc5
completed/success
```

## Audited NO-CHANGE after this fix

Desktop/mobile `material-catalog.html` already protect dynamic material names, units, currencies, comments and timestamps before `innerHTML`; identifiers in links use `encodeURIComponent`, while API errors use `textContent` or explicit escaping.

## Safety / accounting boundaries preserved

- no machine control changed;
- no SSR/start path changed;
- costing remains authoritative server-side;
- pricing history remains append-only;
- no cash/payment data was introduced as a second pricing source;
- no persistence schema or mutation behavior changed.

## Current implementation head before this documentation commit

```text
93876783690fa0fb4c47539c2af240ef77962cc5
```

This documentation commit requires its own exact CI result before the documentation HEAD itself may be called GREEN.
