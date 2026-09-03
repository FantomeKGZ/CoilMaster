# Checkpoint 22 — UI completeness: reports and client card — 2026-09-03

## Scope

Branch: `arduino-ru-lcd-experiment`

Production/source-of-truth `cmp-protocol-v1` was not changed.

This checkpoint records the first confirmed user-facing defects found after the regression-coverage pass. The focus is no longer adding orphaned tests; changes are made only for demonstrated UI/runtime gaps.

## 1. Closed-repair reports — fixed unnecessary full repair scan

Affected pages:

- `firmware/esp32/web/desktop/reports.html`
- `firmware/esp32/web/mobile/reports.html`

The monthly report previously paged through `/api/repairs?cursor=...&limit=32`, then discarded OPEN repairs in the browser. The existing backend already supports `status=CLOSED`, so the report was needlessly scanning OPEN repairs as the registry grew.

Both pages now query:

`/api/repairs?status=CLOSED&cursor=...&limit=32`

The defensive client-side CLOSED check and month check remain in place. Authoritative client, motor and costing lookups remain unchanged; no second financial source of truth was introduced.

Regression coverage:

- new `Tests/Web/check_reports_ui.js`;
- included by the normal `Tests/Web/check_web_assets.js` CMP audit;
- forbids regression back to the unfiltered repair-registry query.

Exact GREEN:

- CMP #4801
- run `33721041652`
- head `cc3beaa48dac48e02bea8f4356c501a0c420e90a`
- `completed/success`

## 2. Client card repair history — fixed invalid status and mobile parity

Affected pages:

- `firmware/esp32/web/desktop/client-details.html`
- `firmware/esp32/web/mobile/client-details.html`
- `Tests/Web/check_client_crm_ui.js`

### Confirmed desktop defect

Desktop client repair history sent:

`status=ALL`

but `/api/repairs` accepts only `OPEN`, `CLOSED`, or no `status` parameter. This could make the `Ремонты и двигатели` block fail with `invalid_repair_status`.

The desktop card now omits `status` when it needs all repairs and keeps the existing client-scoped bounded query with `limit=12` and cursor validation.

### Confirmed mobile parity gap

The mobile client card previously showed contacts, balance and only the first payment page, but had no repair/motor history at all.

Mobile now has:

- `Ремонты и двигатели` section;
- client-scoped bounded repair history (`limit=12`);
- monotonic cursor validation and previous/next paging;
- motor lookup for each shown repair;
- links to motor details, winding history and costing/result;
- payment history previous/next cursor paging (`limit=20`), matching desktop semantics;
- the charged/paid/debt/prepayment balance fields shown consistently.

The backend contract was not weakened: `status=ALL` is forbidden by the CRM regression; all-status history is represented by absence of the `status` parameter.

Regression coverage now verifies desktop/mobile repair and payment paging, repair navigation, cursor guards and the explicit PREPAYMENT model.

Exact GREEN:

- CMP #4804
- run `33721480097`
- head `8cb0309e36a59ccb18962937dda57e9fe63630d9`
- `completed/success`

## Audited NO-CHANGE blocks

### Winding reference

Desktop/mobile `winding-reference.html` remain intentionally reference-only. They load the static `/reference/motor-reference.json`, provide the same query/series/RPM/slot filtering and explicitly do not create a production motor or start winding.

### Winding history

Desktop/mobile `winding-history.html` remain bounded at 20 events per page, enforce monotonic cursors and correctly avoid interpreting one page as the complete run history.

### Motor details

Desktop/mobile motor details expose the same core WORKING/STARTING winding, version history, motor data, repair history, edit and repair actions. No confirmed user-facing gap was found in this pass.

### Repairs catalog

Desktop/mobile repair catalogs correctly implement the UI `ALL` selection by omitting the backend `status` parameter. OPEN/CLOSED filtering, cursor paging and finalization preflight remain intact. The invalid `status=ALL` defect was specific to desktop client details and is now fixed.

## Safety invariants preserved

- no automatic physical START;
- no auto-resume after reboot;
- Arduino remains the sole SSR owner;
- ESP32/Web does not directly control SSR;
- `RUN_COMPLETED` alone does not deduct wire;
- wire writeoff remains explicit/manual and exact-provenance based;
- no report or CRM change creates a second source of costing truth;
- repair close remains protected by the existing finalization preflight.

## Current known GREEN implementation head

`8cb0309e36a59ccb18962937dda57e9fe63630d9`

CMP #4804 / run `33721480097` is exact `completed/success` for that implementation head.

This documentation commit requires its own exact CI result before it may be called GREEN.

## Next work

Continue the real user-facing completeness audit:

1. warehouse and spool create/edit/classification flows;
2. material catalog/create/edit flows and visible stock actions;
3. costing/cash/payment actions and desktop/mobile parity;
4. repair/client/motor navigation edge cases;
5. settings/service pages only where a visible control has a confirmed incomplete action.

Fix only a demonstrated functional gap; otherwise record the audited block as NO-CHANGE.
