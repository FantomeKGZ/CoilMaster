# Checkpoint 26 — client delivery history parity

Date: 2026-09-03
Branch: `cmp-protocol-v1`

## Scope

Closed a desktop/mobile CRM parity gap after the repair-delivery operator flow became available.

Desktop client details already showed CLOSED repair delivery state (`Выдан <дата>` or `Закрыт, не выдан`) using the authoritative immutable delivery API. Mobile client details only showed `Закрыт` and did not read delivery state.

## Fix

`firmware/esp32/web/mobile/client-details.html` now matches desktop behavior for CLOSED repairs:

- reads `GET /api/repairs/delivery?repair_id=...`;
- renders `Выдан <дата>` when a delivery record exists;
- renders `Закрыт, не выдан` when the backend reports no delivery record;
- keeps delivery independent from payment/prepayment/balance state;
- performs no delivery mutation from the client-details page.

The existing client CRM regression was extended instead of creating another standalone/orphan test.

## Commits

```text
8c1b6724c66c27c36967156ddd9485017a4f6ffb  mobile client delivery-history parity
43ccea8a6fdfa0ab53620d5aaa32807c8cf9729a  extend client CRM regression contract
```

## Exact verification

Web implementation HEAD:

```text
8c1b6724c66c27c36967156ddd9485017a4f6ffb
```

Verified:

```text
CMP Protocol Tests #4849
run 33732090065
completed/success

Reference Legacy Import Check #116
run 33732089997
completed/success

ESP32 Build #1880
run 33732090008
completed/success
```

Regression HEAD / current branch HEAD before this documentation commit:

```text
43ccea8a6fdfa0ab53620d5aaa32807c8cf9729a
```

Verified:

```text
CMP Protocol Tests #4850
run 33732132359
completed/success
```

## Safety invariants unchanged

- physical START remains mandatory;
- no auto-resume after reboot;
- Arduino remains SSR owner;
- ESP32/Web do not directly control SSR;
- `RUN_COMPLETED` alone does not write off wire;
- wire writeoff remains manual with exact spool/session/run provenance;
- delivery remains an immutable business event independent from balance.

## Next step

Continue feature-completeness auditing from another proven workflow/parity gap. In particular, verify whether motor repair history and other object-history surfaces expose the same authoritative delivery state after a repair is CLOSED and delivered.