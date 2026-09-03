# Checkpoint 25 — repair delivery operator UI

Date: 2026-09-03
Branch: `cmp-protocol-v1`

## Scope

Closed a concrete repair-workflow gap: the immutable delivery backend already existed, but desktop/mobile operators had no action to record that a CLOSED repair was physically delivered to the client.

## Existing authoritative delivery backend

Delivery remains independent from repair completion and cash/payment state.

Authoritative API:

```text
GET  /api/repairs/delivery?repair_id=...
POST /api/repairs/delivery
```

Durable journal:

```text
/data/workshop/repair-deliveries.ndjson
```

Backend invariants remain unchanged:

- `confirmed=true` is mandatory;
- server derives exact `client_id + motor_id` from authoritative repair identity;
- repair must already be CLOSED;
- exactly one immutable delivery event is allowed per repair;
- outstanding debt does not block delivery;
- delivery history is append-only and is not edited/deleted;
- backup/integrity auditing continues to cover the delivery journal.

## User-facing gap

Before this checkpoint `desktop/repairs.html` and `mobile/repairs.html` could close a repair, but after CLOSED they exposed no operator action for `POST /api/repairs/delivery`.

Cash UI intentionally remained read-only with respect to delivery and must not become a delivery gate.

## Fix

CLOSED repair cards now:

1. read the exact delivery state with `GET /api/repairs/delivery?repair_id=...`;
2. show the recorded delivery date when already delivered;
3. show `Отметить выдачу` only when the backend returns delivery-not-found;
4. require explicit operator confirmation;
5. send `repair_id + delivered_at + confirmed=true` to the canonical POST endpoint;
6. reload immutable delivery state after a successful append.

Operator wording explicitly states that delivery is one-time and independent from balance.

No payment/balance lookup is used as a delivery precondition.

## Commits

```text
c8a0e90887c33d3eb3e4e6d6c9cf383e3df60721  desktop repair delivery action
24ca25474a29a191fc938af8116b0df367de2a9c  mobile repair delivery action
c542ed6b017e2fcb16a43675da35c7686761b60d  preserve desktop HTML escaping
6b08a01f01fb241157a715108ee5aa73f24aa975  delivery UI regression contract
f77d50bc4547ca2b5298545426fb93da46fa863f  run delivery contract from Web umbrella
```

## Regression protection

`Tests/Web/check_repair_delivery_ui.js` protects:

- desktop/mobile delivery state reads;
- canonical delivery POST;
- `confirmed=true`;
- CLOSED ownership of the operator action;
- one-time/balance-independent operator semantics;
- backend CLOSED-only and duplicate-delivery guards;
- absence of cash balance gating;
- absence of PUT/PATCH/DELETE delivery shortcuts.

The test is reachable through `Tests/Web/check_web_assets.js` and therefore guarded by the orphan-contract meta-audit.

## Exact verification

Cumulative production Web code HEAD:

```text
c542ed6b017e2fcb16a43675da35c7686761b60d
```

Verified:

```text
CMP Protocol Tests #4845
run 33731499218
completed/success

Reference Legacy Import Check #115
run 33731499279
completed/success

ESP32 Build #1879
run 33731499232
completed/success
```

Regression HEAD:

```text
f77d50bc4547ca2b5298545426fb93da46fa863f
```

Verified:

```text
CMP Protocol Tests #4847
run 33731614779
completed/success
```

## Safety invariants unchanged

This UI records a business delivery event only.

Still enforced:

- physical START only;
- no auto-resume after reboot;
- Arduino owns SSR;
- ESP32/Web do not directly control SSR;
- `RUN_COMPLETED` alone does not write off wire;
- manual wire writeoff keeps exact spool/session/run provenance.

## Next step

Continue feature-completeness from another proven active workflow/runtime gap. Repair creation, finalization/close and immutable delivery now form an operator-accessible flow; do not move delivery into Cash or add a balance gate.
