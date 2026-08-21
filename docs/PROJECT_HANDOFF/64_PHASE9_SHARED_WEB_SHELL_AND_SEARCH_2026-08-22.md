# Checkpoint 64 — Phase 9 shared web shell and search

Date: 2026-08-22
Branch: `cmp-protocol-v1`

## Status

The implementation scope of Phase 9 from
`40_UI_HARDWARE_SETTINGS_AND_JOB_LIFECYCLE_PLAN_2026-08-20.md` is complete at repo level.

This checkpoint does **not** claim the current HEAD is CI-green. Exact current push workflow results were not available through the connector at the time of this checkpoint, so current-head automated verification remains `NOT VERIFIED` until an exact Actions run with matching `head_sha` is inspected.

The last previously verified production CI baseline remains separate historical evidence; do not reuse that green result as proof for the Phase 9 commits below.

## Phase 9 coverage

The seven planned items are now represented in current code/contracts:

1. unified desktop/mobile navigation — centralized shared shell navigation;
2. FTP page shell — shared shell is injected centrally into desktop/mobile HTML, including FTP settings;
3. global clock — device time comes from `/api/system/time`, with client-side ticking and infrequent re-sync rather than one request per second;
4. firmware/web version — read-only `/api/system/build` exposes build identity used by the shell;
5. shared toasts/errors — `CMApp.toast` and shared error text handling;
6. global search/recent/breadcrumbs — real motor/client/repair search, bounded recent items and breadcrumbs;
7. UI contract tests — `Tests/Web/check_shared_app_shell_contracts.js` is wired into CMP Protocol Tests.

## Real global search completion

The previous Phase 9 shell could search motors through the real backend but used catalog-navigation fallbacks for some client/repair text searches. That gap is now closed.

New read-only endpoints:

```text
GET /api/search/clients?q=...&limit=...&cursor=...
GET /api/search/repairs?q=...&limit=...&cursor=...
```

Implementation properties:

- existing `/api/clients?phone=...` and `/api/repairs?...` semantics are unchanged;
- client search matches existing client records by name, phone, normalized phone, comment and id;
- repair search matches repair/client/motor ids plus received date, complaint and comment;
- repair search keeps authoritative current OPEN/CLOSED decoration through existing status resolution;
- readers are bounded by `MaxListPageSize` and cursor-based;
- search is read-only and does not append or rewrite workshop NDJSON;
- shared shell performs no physical-control, SSR or JOB write action.

Relevant commits in this final Phase 9 block:

```text
15981c36ef7023c71e2bfdfbff2d970315896e37  Declare read-only registry search readers
8d28e9ced7e9f02b9fcf7dc13c892352a3371d6e  Add bounded registry search readers
17c2920d92008fb6dbab2a50c216e0263212a172  Declare registry search HTTP handlers
754147cb5429cdf8ac3e4812fae4c7e77f0784a6  Expose read-only registry search endpoints
eab2390f413abb241935af0d119a5893cb43e712  Use real client and repair global search
0233206fd21f0b5eae668fceef7c5dea7a535574  Guard real global registry search contracts
```

Earlier Phase 9 shell commits include the shared shell foundation, centralized StaticSiteServer injection, build identity endpoint and shared contract workflow step.

## Verification state

At checkpoint creation:

```text
Phase 9 implementation: COMPLETE
shared shell contract coverage: PRESENT IN REPO
current HEAD CI result: NOT VERIFIED
hardware acceptance: NOT IMPLIED
```

`fetch_commit_workflow_runs` returned no usable push runs because that connector path only exposes the supported subset, and combined commit statuses were empty. Therefore no green claim is made for `0233206fd21f0b5eae668fceef7c5dea7a535574` or for this documentation-only successor commit.

## Mandatory next work after the plan

The user explicitly requested that temporarily deferred work be resumed after this plan rather than lost or replaced by old completed tasks.

After Phase 9 implementation closure, return immediately to:

1. `63_FULL_CODE_AUDIT_2026-08-22.md`;
2. the open Arduino finding `A-002` — strict canonical validation of incoming production JOB fields/type tokens, with regression coverage;
3. targeted ESP32<->Arduino desync/recovery review for `JOB / JOB_ACK / JOB_CANCEL / JOB_CANCEL_ACK / ALL_CLEAR / timeout / reboot / replay` state transitions;
4. persisted ESP32 job-state reconciliation in lost-ACK / late-ALL_CLEAR / reboot cases;
5. then the remaining ESP32, Web, tests/CI and documentation sections of checkpoint 63.

The generic JOB cancel/recovery feature is already implemented and must **not** be re-created from scratch. Reopen code only for a concrete defect, inconsistent transition, missing fail-closed validation or reproducible hardware regression found by the targeted audit.

## Safety boundary remains unchanged

```text
physical START only physical/local
no automatic START between repeats
no auto-resume after reboot
Arduino owns SSR
ESP32/Web do not directly drive SSR
RUN_COMPLETED never auto-writes off material
manual writeoff requires exact source_session_id + source_run_id
spool_id optional only for approved KG_FIRST unallocated/manual path
exact spool provenance retained when a spool is used
backup restore operator-only, transactional and fail-closed
no automatic production-data deletion
```

## Continuation rule

For current work selection, checkpoint 64 closes the Phase 9 implementation queue and hands control back to checkpoint 63. Older numbered checkpoints remain implementation history/evidence unless a current defect explicitly points back to them.
