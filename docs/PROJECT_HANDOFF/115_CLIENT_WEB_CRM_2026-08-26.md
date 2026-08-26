# Checkpoint 115 — Client Web CRM

Date: **2026-08-26**  
Branch: **`cmp-protocol-v1`**

## Status

**SOFTWARE GREEN**.

## Implemented

Desktop Client Web is now split into distinct catalog/create/details responsibilities.

### Catalog-only clients page

`firmware/esp32/web/desktop/clients.html`

- read-only bounded `/api/clients` paging;
- phone search;
- links to exact client card;
- link to create a repair for the selected client;
- no inline `POST /api/clients` form.

### Dedicated client creation

`firmware/esp32/web/desktop/client-new.html`

- creates client only through existing `POST /api/clients`;
- does not create repair or motor linkage;
- stores selected client for the existing repair workflow;
- offers explicit handoff to client card or new repair.

Motor ownership remains historical through repair records. No mutable permanent `client_id` is added to physical motor identity.

### Client card

`firmware/esp32/web/desktop/client-details.html?client_id=...`

Reads existing GREEN APIs only:

```text
GET /api/clients/by-id?client_id=...
GET /api/repairs?client_id=...&status=ALL&limit=12
GET /api/motors/by-id?motor_id=...
GET /api/repairs/delivery?repair_id=...
GET /api/payments/balance?client_id=...
GET /api/payments?client_id=...&limit=20
```

Card shows:

- client identity/contact/comment;
- bounded repair history;
- motors brought through repair history;
- OPEN/CLOSED state;
- immutable delivery state/date when present;
- charged / paid / debt / credit;
- bounded append-only payment history.

Client card is read-only: it does not POST payments, deliveries, repairs, machine jobs or material movements.

### Repairs page cleanup

`firmware/esp32/web/desktop/repairs.html`

Removed the duplicate inline client-creation form and handler. Repair intake now links to `/desktop/client-new.html` for new clients while still selecting exact existing client IDs for repair creation.

## Cash / delivery contract alignment

Client card was checked against the actual checkpoint 111/112 runtime contracts:

- delivery GET returns delivery fields at top level;
- cash event kind is `kind`;
- client balance uses `charged_minor`, `paid_minor`, `debt_minor`, `credit_minor`;
- delivery remains independent of zero balance.

## Permanent regression

New:

```text
Tests/Web/check_client_crm_ui.js
```

It is permanently invoked by:

```text
Tests/Web/check_motor_details_ui.js
```

Protected contracts include:

- clients catalog is read-only;
- dedicated client creation page exists;
- client card uses bounded repair/payment paging;
- motor relation is historical through repairs;
- client card uses payment/balance/delivery reads only;
- duplicate repair-page client create form stays removed;
- delivery/cash independence remains explicit.

## CI evidence

```text
CMP Protocol Tests 32936343060 / SUCCESS
```

All 67 permanent host/audit steps completed successfully, including Web JavaScript/navigation, Client CRM regression through the motor-details audit, release safety, job preparation, warehouse/material, backup and Hall contracts.

No ESP32 firmware source changed in this Client Web block; latest firmware build evidence from checkpoint 114 remains valid for firmware source, while this block is Web/host-regression only.

## Temporary helper workflows

Guarded patch helpers used for the large one-line Web files were converted to manual read-only archived stubs:

```text
.github/workflows/client-web-repairs-patch.yml
.github/workflows/client-details-contract-patch.yml
```

They no longer run on push and cannot mutate repository contents.

## Safety unchanged

- physical START remains local-only;
- Arduino remains SSR owner;
- Web client pages do not create machine-control shortcuts;
- `RUN_COMPLETED` never auto-deducts material;
- warehouse ISSUE remains explicit operator action;
- cash remains append-only and never controls machine/warehouse state;
- delivery does not rewrite or erase debt/payment history;
- exact spool production contract remains until coordinated migration.

## NEXT

Create dedicated `cash.html` using checkpoint 112 payment/balance APIs. Keep `costing.html` focused on cost/price/margin and do not merge cash mutations into costing.
