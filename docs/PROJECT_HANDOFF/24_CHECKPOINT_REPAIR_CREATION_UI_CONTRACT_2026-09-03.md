# Checkpoint 24 — repair creation UI regression contract

Date: 2026-09-03

## Scope

Closed a concrete regression-coverage gap around desktop/mobile repair creation on active branch `cmp-protocol-v1`.

## Existing runtime contract

Both repair creation pages use the canonical transactional endpoint:

```text
POST /api/repairs
```

The backend endpoint is already protected by `RepairIntakeCoordinator`; it must not bypass transactional intake/snapshot persistence.

## Added protection

New regression:

```text
Tests/Web/check_repair_new_ui.js
```

It protects desktop/mobile repair creation against drift in:

- required `client_id`, `motor_id`, `received_at` fields;
- complaint/comment capture;
- exact client and motor lookup endpoints;
- canonical `POST /api/repairs` only;
- required `repair_id` response;
- `cm-active-repair` handoff;
- duplicate-submit/single-flight guard;
- correct desktop/mobile return navigation;
- no parallel stale `/api/repairs/intake` or `/api/repairs/create` mutation path.

The test is reachable from the main Web umbrella:

```text
Tests/Web/check_web_assets.js
```

## Commits

```text
ca33419ae354f0c769a301239ab43a722248ed59  add repair creation UI contract
330c6f3ab4d75884dc98ae27deee957f39646f09  run repair creation UI contract from umbrella
```

## CI evidence

Intermediate commit `ca33419...` intentionally exposed the new meta-audit:

```text
CMP Protocol Tests #4840
run 33730288254
completed/failure
```

Exact failure:

```text
Orphaned Web regression contracts: check_repair_new_ui.js
```

No production/runtime regression was reported by that run; all other executed audits passed. The failure was the expected reachability guard detecting the newly added test before it was connected.

Final code HEAD:

```text
330c6f3ab4d75884dc98ae27deee957f39646f09
```

Exact verification:

```text
CMP Protocol Tests #4841
run 33730367326
completed/success
```

## Safety invariants unchanged

No physical START, SSR, UART, warehouse, costing, wire writeoff or repair-finalization behavior changed.

Still enforced:

- physical START only;
- no auto-resume after reboot;
- Arduino owns SSR;
- ESP32/Web do not directly control SSR;
- `RUN_COMPLETED` alone does not write off wire;
- manual wire writeoff keeps exact spool/session/run provenance.

## Next step

Continue only from another proven user-facing/runtime gap. Repair creation runtime and its desktop/mobile regression coverage are now closed unless new evidence appears.
