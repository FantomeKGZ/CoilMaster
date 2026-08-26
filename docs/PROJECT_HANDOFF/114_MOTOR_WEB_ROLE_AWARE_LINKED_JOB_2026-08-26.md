# 114 — Motor Web role-aware linked job — 2026-08-26

Status: **SOFTWARE GREEN**

Repository: `FantomeKGZ/CoilMaster`  
Branch: **`cmp-protocol-v1`** only. `main` remains frozen at the stable pre-CRM snapshot.

## Scope closed

Motor Web now completes the versioned winding workflow without introducing a second machine-control path.

### Motor card

`firmware/esp32/web/desktop/motor-details.html`

- current WORKING / STARTING winding version;
- multi-conductor strings;
- bounded winding-version history;
- immutable `AS_RECEIVED` comparison per repair;
- historical after-repair version matched by exact `source_repair_id`;
- legacy repair without snapshot is explicitly distinguished from corruption;
- OPEN repairs expose navigation links:
  - `winding-job.html?repair_id=...&role=working`
  - `winding-job.html?repair_id=...&role=starting`
- CLOSED repairs do not expose new-job role links;
- motor card never POSTs a job directly and never controls SSR.

AS_RECEIVED targeted verification:

```text
CMP 32933183958 / SUCCESS
```

### Linked job role owner

`CM_JobLinkageResolver` now owns authoritative role selection for linked production jobs:

- latest motor winding version wins;
- WORKING uses exact `working_program + working_repeat_target`;
- STARTING requires `starting_present=true` and exact `starting_program + starting_repeat_target`;
- no version -> legacy WORKING fallback only;
- legacy STARTING fails closed;
- absent legacy `repeat_target` retains historical default `1`;
- present but malformed legacy `repeat_target` fails closed.

The existing repair->motor OPEN linkage checks remain unchanged.

### Server-side linked job validation

`handleCreateJob()` continues using the existing linked `/api/jobs` production path.

Before immutable job snapshot/state/spool-selection persistence and before UART:

```text
requested role
  -> resolver exact program + repeat target
  -> exact program comparison
  -> exact repeat_target comparison
  -> existing exact spool selection checks
  -> snapshot -> state -> spool selection -> DELIVERING -> UART
```

New mismatch error:

```text
repeat_target_does_not_match_motor_role
```

Guarded source patch evidence:

```text
32933386413 / SUCCESS  role-aware main.cpp call
32933692178 / SUCCESS  repeat-target validation in main.cpp
```

### Linked winding job UI

`firmware/esp32/web/desktop/winding-job.html`

- reads `/api/motors/winding/latest`;
- exposes only real versioned role data;
- legacy motor has WORKING only;
- STARTING option is disabled when absent;
- an explicit `?role=starting` without STARTING does **not** silently fall back to WORKING;
- role program is readonly;
- role repeat target is readonly;
- selected values are convenience/UI data only; ESP32 revalidates exact role/program/repeat server-side;
- existing exact spool selection remains mandatory;
- page still POSTs only through existing `/api/jobs` path.

Guarded UI patch evidence:

```text
32933873111 / SUCCESS
32934032276 / SUCCESS  safe Motor card role links
```

One-shot patch workflows were converted after use to manual `workflow_dispatch`, `contents: read`, no-mutation archive workflows because connector deletion was blocked.

## Permanent regression coverage

- `Tests/Web/check_linked_job_winding_role.js`
- `Tests/Web/check_winding_job_role_ui.js`
- existing `check_job_preparation_transaction.js` invokes both role audits;
- `check_motor_details_ui.js` covers AS_RECEIVED comparison and safe OPEN-repair role navigation.

A transient CMP failure `32934092549` was caused only by a wrongly escaped textual regression expectation for `starting_present`; production code was not the cause. The expectation was corrected to robust source tokens.

## Final verification

```text
ESP32 Build 32934092563 / SUCCESS
CMP Protocol Tests 32934323481 / SUCCESS
```

The ESP32 run is on normal connector source commit `da5d7271ba69e373599360550e81e1cf860f7a1a`, after the guarded `main.cpp` role/repeat patches, so it compiles the integrated production source tree.

## Safety retained

- physical START remains local-only;
- Web role links only navigate to linked-job preparation;
- no Web SSR path added;
- every repeat still requires physical START;
- `RUN_COMPLETED` remains non-mutating for wire/material;
- exact `spool_id` production contract remains until the later coordinated migration;
- closed repair cannot create a linked job;
- wrong/missing role/program/repeat fails closed before job persistence/UART.

## NEXT

1. Client Web redesign:
   - `clients.html` catalog-only;
   - separate `client-new.html`;
   - `client-details.html?client_id=...` with motors through repair history, open/closed repairs, payments/balance and delivery history.
2. Dedicated `cash.html` on checkpoint 112 APIs.
3. Coordinated spool -> Material Request wire migration only as one complete backend/Web/test change.
