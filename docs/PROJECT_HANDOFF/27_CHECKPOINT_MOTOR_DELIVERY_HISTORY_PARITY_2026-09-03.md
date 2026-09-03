# Checkpoint 27 — motor delivery history parity

Date: 2026-09-03
Branch: `cmp-protocol-v1`

## Scope

Closed a concrete object-history gap after repair delivery became operator-accessible.

Both desktop and mobile motor cards previously showed CLOSED repairs only as `Закрыт`, even when the immutable delivery journal already contained a delivery event.

## Fix

`desktop/motor-details.html` and `mobile/motor-details.html` now read authoritative delivery state for CLOSED repairs only:

- `GET /api/repairs/delivery?repair_id=...`;
- delivery exists -> show `выдан: <дата>`;
- delivery not found -> show `закрыт, не выдан`;
- other read failure -> show delivery state as unavailable instead of inventing state.

Motor details remain read-only with respect to delivery. The canonical delivery mutation remains in `repairs.html`; no payment/balance condition was added.

Repair-history rendering now awaits the delivery-state read before rendering each CLOSED repair, preserving a coherent historical label.

## Commits

```text
c5faa4e9ae1eb09dedfcf91d753cbfdd3de7bc78  desktop motor delivery history
a74deb7c913a4725d79d82dd5c5902dd74fcbe4c  mobile motor delivery history
4e04c144ceb00602eaa7b8b5bf534137326dc458  extend motor-details regression contract
```

## Regression protection

`Tests/Web/check_motor_details_ui.js` now protects:

- desktop/mobile authoritative delivery GET;
- `выдан` and `закрыт, не выдан` history semantics;
- awaited delivery lookup before repair rendering;
- absence of delivery POST from motor-details;
- existing bounded repair/version history;
- existing AS_RECEIVED comparison;
- existing physical START/SSR/direct-job safety contracts.

## Exact verification

Cumulative Web code HEAD:

```text
a74deb7c913a4725d79d82dd5c5902dd74fcbe4c
```

Verified:

```text
CMP Protocol Tests #4853
run 33732645388
completed/success

Reference Legacy Import Check #118
run 33732645370
completed/success

ESP32 Build #1882
run 33732645361
completed/success
```

Regression HEAD:

```text
4e04c144ceb00602eaa7b8b5bf534137326dc458
```

Verified:

```text
CMP Protocol Tests #4854
run 33732693018
completed/success
```

## Safety invariants unchanged

- physical START remains mandatory;
- no auto-resume after reboot;
- Arduino remains SSR owner;
- ESP32/Web do not directly control SSR;
- `RUN_COMPLETED` alone does not write off wire;
- wire writeoff remains manual with exact spool/session/run provenance;
- delivery remains append-only and independent from balance.

## Next step

Continue feature-completeness auditing from another proven user-visible workflow gap. Prefer existing authoritative APIs and read-only parity fixes over adding duplicate state or new persistence.