# Checkpoint 149 — client editing + client prepayment — 2026-08-28

Working branch: `arduino-ru-lcd-experiment`  
Production/source-of-truth remains `cmp-protocol-v1` at `28c7917a906bc9b15736369e8986d0e0c354ab8c`.  
Do not transfer this experiment block to production without an explicit request.

## Result

Client editing and simple client-level prepayment are implemented and verified GREEN.

### Client editing

Authoritative client identity remains `RepairRegistry` with stable `client_id`.

Base client records are not destructively rewritten. Edits use append-only revisions in `client-revisions.ndjson`, following the existing motor-revision pattern. The latest revision is resolved consistently by client by-id, paged list and search paths, so historical repairs retain their original stable `client_id` relationship.

HTTP/UI:

- `POST /api/clients/update`
- exact nonzero `client_id`
- required name and valid normalized phone
- optional comment
- response declares `revision_history: APPEND_ONLY`
- desktop and mobile client cards expose edit controls and reread authoritative client state after save.

Relevant implementation lineage includes client revision commits through `4105ae14121ed52b3fd1954a4ffe0dec64e7803c` plus the client-card UI commits below.

## PREPAYMENT architecture

No new cash ledger was created.

Existing authoritative cash journal remains:

```text
/data/workshop/repair-payments.ndjson
```

`PREPAYMENT` is a new client-scoped cash event in the existing `CashPaymentStore`.

Hard rules:

```text
kind = PREPAYMENT
direction = ADD
repair_id = 0
client_id = exact existing client
amount_minor > 0
confirmed = true
```

`repair_id` and `corrects_event_id` are rejected on PREPAYMENT HTTP creation. Client existence is validated through authoritative `RepairRegistry` before append and again by integrity audit semantics.

Ordinary `PAYMENT` / `CORRECTION` remain repair-scoped and retain their exact repair/client/costing validation.

### No automatic repair offset

Prepayment is deliberately NOT auto-applied to a repair.

Client cash totals now keep two separate buckets:

- `paid_minor` — net payments/corrections attached to repairs;
- `prepayment_minor` — client prepayment balance.

Repair totals/history skip `PREPAYMENT`. Repair debt remains calculated from repair charges versus repair `paid_minor` only. `RepairCosting` is unchanged and prepayment never changes material/wire/labour costing.

A future transfer/application of prepayment to a specific repair, if ever added, must be an explicit separate operator action with its own provenance; do not silently change this behavior.

## UI

Desktop and mobile `client-details.html` now provide:

- edit name / phone / comment;
- separate client balance display including `prepayment_minor`;
- form `Зачислить предоплату`;
- CASH / CARD / TRANSFER / OTHER method selection;
- explicit `confirmed=true` POST to `/api/payments` with `kind=PREPAYMENT`;
- authoritative balance/history reread after success;
- payment history labels prepayment separately and shows that it has no repair linkage.

## Main commits in this block

```text
c059cb4360caf4214f0df49b28b6c9b17d7fd538  separate client prepayment totals
7ad520f35cb5db53a5fb33bbd5ef7d22f3600b67  allow client-scoped PREPAYMENT in cash journal
9f1f247da815d881428c662c1ff36e291f096480  keep PREPAYMENT separate in client totals
47f657d19176168454fd14aa0d28c9ba1479d580  audit client prepayment provenance
cfdd2333894161f9720cea9ca53e57dd7324b740  HTTP explicit PREPAYMENT
7517fa4afb7dae7de7530ea83524be20b048e615  desktop client edit + prepayment UI
d47b2f5a088566af33f20e805104206b9c167fe2  mobile parity
7e73b7590ecd0d38f3ce4cb65208a8f6bde46eb1  cash/prepayment contract coverage
58b1aff87434dbb38a16327c7929b2a61b4befc9  align stale CRM detail audit with writable client card
```

## Verification

Runtime/UI code SHA `d47b2f5a088566af33f20e805104206b9c167fe2`:

```text
ESP32 Build #1699
run 33162070105
completed / success

Arduino RU LCD Build #123
run 33162070106
completed / success
```

Final host contract SHA `58b1aff87434dbb38a16327c7929b2a61b4befc9`:

```text
CMP Protocol Tests #3863
run 33162276329
completed / success
```

CMP #3862 / run `33162135407` failed only because the older `check_client_crm_ui.js` still asserted that client-details must be read-only and required the old credit display. The new cash/prepayment contract itself passed in that run. The stale CRM assertion was corrected in `58b1aff8...`; #3863 then completed success. No runtime rollback was required.

User-provided prior baseline was also GREEN: CMP #3854 / ESP32 #1692 / Arduino RU LCD #116.

## Safety / boundaries unchanged

- no automatic physical START;
- no auto-resume after reboot;
- Arduino remains sole SSR owner;
- Web/ESP32 do not directly control SSR;
- RUN_COMPLETED still never writes off wire automatically;
- exact RUN_WIRE spool/session/run provenance remains mandatory;
- material ledger / warehouse / costing safety paths are unchanged;
- PREPAYMENT is financial CRM data only and does not modify costing or stock.

## Next work

Return to the active repair-material plan after this requested CRM block. Do not add automatic prepayment allocation. If client financial work is extended later, prefer explicit append-only events and exact provenance over destructive balance edits.