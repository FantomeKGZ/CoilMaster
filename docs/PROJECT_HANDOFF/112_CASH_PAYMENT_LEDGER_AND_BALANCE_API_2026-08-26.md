# Checkpoint 112 — Cash payment ledger and balance API

Date: **2026-08-26**  
Branch: **`cmp-protocol-v1`**

## Status

**SOFTWARE GREEN**.

This checkpoint adds the cash/payment persistence layer and bounded balance APIs without changing the existing repair-price source of truth.

## Authoritative price contract

Cash does **not** persist repair price.

`RepairCosting::load(repair_id, summary)` remains authoritative for:

```text
clientPriceMinor
currency
```

Existing price history remains `/data/repairs/pricing.ndjson`.

## New append-only cash journal

```text
/data/workshop/repair-payments.ndjson
```

Implemented by:

```text
firmware/esp32/src/CM_CashPaymentStore.h
firmware/esp32/src/CM_CashPaymentStore.cpp
firmware/esp32/src/CM_CashPaymentStoreLookup.cpp
```

Events:

```text
PAYMENT    -> ADD only
CORRECTION -> ADD | SUBTRACT
```

Fields include:

```text
cash_event_id
kind
direction
repair_id
client_id
amount_minor
currency
occurred_at
method
comment
corrects_event_id (optional for correction)
```

`client_id` is derived from authoritative repair identity by the Web layer. Old records are never rewritten or deleted.

## Cash invariants

- amount is positive integer minor units;
- PAYMENT cannot carry a correction target;
- correction target, when supplied, must exist and belong to the same `repair_id + client_id`;
- SUBTRACT cannot make effective paid total negative;
- all cash events for a repair/client aggregate must use one currency;
- cash journal never stores `client_price_minor`;
- payment remains allowed after repair CLOSED and after delivery, so post-delivery debt can be paid;
- delivery remains independent of payment balance and is not blocked by debt.

## API

```text
GET  /api/payments?repair_id=...
GET  /api/payments?client_id=...
POST /api/payments
GET  /api/payments/balance?repair_id=...
GET  /api/payments/balance?client_id=...
```

POST requires:

```text
confirmed=true
repair_id
amount_minor
occurred_at
```

Optional:

```text
kind
correction direction
corrects_event_id
currency (must match RepairCosting)
method = CASH | CARD | TRANSFER | OTHER
comment
```

## Balance reads

Repair balance returns:

```text
repair_id
client_id
motor_id
charged_minor
paid_minor
debt_minor
credit_minor
payment_event_count
currency
```

Client aggregate balance returns:

```text
client_id
charged_minor
paid_minor
debt_minor
credit_minor
repair_count
payment_event_count
currency
```

Client charges are computed by bounded paging through authoritative `RepairRegistry::appendRepairsPageJson()` and loading `RepairCosting` for each repair. Client payments are aggregated in a single pass over the append-only cash journal.

## Runtime integration

Cash runtime is registered from `CM_MaterialLedgerWeb.cpp`, reusing the existing production `RepairCosting` instance:

```text
RepairCosting
RepairRegistry
CashPaymentStore
CashPaymentWeb
```

Routes are registered only after registry/payment-store initialization succeeds.

## Backup / integrity

Backup whitelist now includes:

```text
repair-payments -> /data/workshop/repair-payments.ndjson
```

`CM_CashPaymentIntegrityAudit` validates:

- canonical increasing event IDs;
- event kind/direction/amount/timestamp/method;
- repair exists;
- stored client matches exact repair client;
- currency matches authoritative RepairCosting;
- correction target belongs to the same repair/client;
- no destructive repair behavior.

Stable backup fails closed with:

```text
cash_payment_unstable_or_invalid
```

## Main implementation commits

```text
d904770dea238aeffee9dc9e68a1189f548cf1cc  production cash runtime registration
f25ed6a801dd69dc8dbc420d935d2c2c7283f998  backup export/integrity integration
1e1b03d95c8624e2a220b0a4183f3fda3279474d  correction identity + client totals implementation
eac97f9c59fdd0a7ca3e53c73ad4748aa0d1933e  correction target integrity audit
8eb8352e333e81a008c3d2fb702aa15f903a9b5d  permanent final cash regression contract
```

## Verified CI evidence

Final relevant checks:

```text
CMP Protocol Tests 32928743465 / SUCCESS
ESP32 Build         32928706196 / SUCCESS
```

The ESP32 run is on `eac97f9...`, the last production-source commit. Current HEAD `8eb8352...` changes only the permanent host regression test, so that ESP32 result covers the final firmware source state.

Earlier foundation evidence:

```text
CMP 32928282206 / SUCCESS
cash runtime one-shot 32928089937 / SUCCESS
```

## Transient development failures

These are retained for auditability and are **not** final-state failures:

- ESP32 `32928605210` occurred on an intermediate commit where Web declarations referenced new payment lookup methods before their implementation landed.
- CMP `32928706078` occurred after the integrity implementation changed but before the permanent regression expectation was updated.
- obsolete one-shot hardening workflow failures were patch-mechanism artifacts; those workflows were archived as manual, non-mutating stubs.

Final CMP + ESP32 evidence above is GREEN.

## Safety unchanged

- no physical auto START;
- no auto-resume;
- Arduino owns SSR;
- `RUN_COMPLETED` never deducts material automatically;
- warehouse material ISSUE remains explicit;
- cash events do not trigger machine/warehouse actions;
- payment balance does not erase or rewrite delivery/run/material evidence.

## NEXT

With motor/winding, repair snapshots, Material Request warehouse flow, delivery, and cash backend foundations available, continue the approved Web/CRM redesign:

1. Motor Web (`motors.html`, `motor-new.html`, `motor-details.html`).
2. Client Web (`clients.html`, `client-new.html`, `client-details.html`) using repair/payment/delivery reads.
3. Dedicated `cash.html` UI after client/motor cards are wired to the new APIs.
4. Later coordinated spool -> Material Request wire migration; do not partially remove the current exact-spool production contract.
