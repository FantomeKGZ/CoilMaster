# Checkpoint 168 — FTP/Web recovery and backup/settings parity

Date: **2026-08-31**  
Branch: **`arduino-ru-lcd-experiment`**  
Production/source-of-truth: **`cmp-protocol-v1`** — unchanged.

## Result

The FTP/Web recovery plus backup/settings feature-completeness audit is closed as **NO-CHANGE runtime**. No backend or production behavior change is required.

The audit confirmed:

- desktop and mobile FTP settings use the same shared controller;
- operator FTP controls resolve `/api/ftp/status` plus POST `/api/ftp/` + explicit `start` / `stop` action;
- FTP storage remains confined to `/web`;
- upload replacement uses a temporary `.part` file before rename to the final path;
- FTP mutations remain guarded by the runtime activity safety probe;
- local-network access remains restricted by the server-side subnet check;
- automatic FTP recovery starts only when required Web entrypoints are absent and stops when `/web` becomes usable;
- web readiness requires root, desktop and mobile entrypoints;
- remote backup settings expose the same host/port/user/directory/retention/enabled controls on desktop/mobile through one shared controller;
- retention remains bounded to `1..30`;
- schedule support is shared and complete: enabled + hour + minute, default `02:00`, preserved when omitted by legacy clients;
- a blank password preserves the already stored credential and API responses keep credentials hidden;
- remote-backup configuration/test use `/api/backup/remote/configuration` and `/api/backup/remote/test`;
- no physical START, SSR, RUN evidence, Warehouse, wire-writeoff or backup-restore mutation semantics were changed.

## Restore UI reachability boundary

The apparent absence of restore controls in static `desktop/backup.html` and `mobile/backup.html` is intentional, not a missing feature.

The current boundary is:

```text
static/offline backup page -> read-only export only
live ESP32-served /backup.html -> CM_StaticSiteServer runtime-injects /shared/backup-remote-upload.js
```

This preserves safe offline/read-only behavior while making the full operator restore workflow available only on the live device site.

The live shared restore controller and backend are route-aligned for:

```text
upload
status
batch
retention
batch-status
inspection
inspection-status
staging
staging-status
staging DELETE/discard
restore-plan
restore-plan-status
rollback-snapshot
rollback-snapshot-status
apply-preflight
apply-preflight-status
apply
apply-status
```

The operator sequence remains explicit:

```text
inspection -> staging -> restore plan -> rollback snapshot -> apply preflight -> typed APPLY confirmation -> apply
```

Discard remains separately available. Stale/failure state remains fail-closed. No automatic restore or reboot continuation was introduced.

Two temporary direct static-helper includes were tested during the audit and immediately reverted after the existing runtime-injection boundary was confirmed. Final desktop/mobile backup HTML is restored to the intended static read-only form; there is no duplicate restore-helper initialization in the final state.

## Regression locks

FTP/Web recovery contract:

```text
Tests/Web/check_web_recovery_ftp_contracts.js
```

Backup/settings and live/offline restore parity contract:

```text
Tests/Web/check_remote_backup_ui_parity.js
```

Both are invoked from:

```text
Tests/Web/check_ci_trigger_contracts.js
```

The new remote-backup parity regression protects:

- no static inclusion of `backup-remote-upload.js` in desktop/mobile backup pages;
- live `/backup.html` helper injection by `CM_StaticSiteServer`;
- delayed stale-guard startup and no read-only polling;
- UI/backend route parity for the complete operator restore chain;
- typed `APPLY` confirmation in UI and backend;
- staging discard parity;
- desktop/mobile settings parity;
- retention bounds;
- schedule fields;
- password-preservation and credential-hiding semantics.

## Verification chain

FTP contract chain:

```text
226cc65a82cfc42bc172fe27b53153e669062812  test(web): lock FTP recovery contracts
CMP Protocol Tests #4740 / run 33371841283 / SUCCESS

d0ff4b0cd4fc5197511012f6baa4dd033caa2ac7  test(web): run FTP recovery contract audit
CMP Protocol Tests #4741 / run 33371920243 / FAILURE
reason: regression expected literal /api/ftp/start while production correctly builds /api/ftp/ + action

8a3eadd260b9c1d33c9cf44e5fbd203e261b7e69  test(web): align FTP action contract with shared controller
CMP Protocol Tests #4743 / SUCCESS
```

Restored intended runtime-injection boundary:

```text
75b36c9875fd1fd271b433e8db4b940d43835fea  revert(web): preserve mobile runtime restore injection boundary
CMP Protocol Tests #4748 / run 33372615230 / SUCCESS
ESP32 Build #1850 / run 33372615249 / SUCCESS
```

Remote-backup parity contract chain:

```text
8eced27f043140744cdb6c630c040f5d876f8250  test(web): lock remote backup UI parity
ab0c1cbcf95091561d4b507bfbf383889ed4502b  test(web): run remote backup parity audit
CMP Protocol Tests #4750 / run 33372914424 / FAILURE
reason: regression matched an escaped C++ JSON literal too strictly; production still explicitly returned credentials_exposed=false

fbd292f086cf77a01825095c32676ac461f88f9c  test(web): align credential hiding contract
CMP Protocol Tests #4751 / run 33373033715 / SUCCESS
```

`#4741` and `#4750` are intermediate **test-contract failures**, not runtime failures. Their corrections changed only regression assertions.

Exact code/test HEAD `fbd292f086cf77a01825095c32676ac461f88f9c` is CMP-confirmed GREEN by `#4751`. This documentation commit is newer and needs its own exact SUCCESS before it may be called GREEN.

## Next repo-reviewable block

Backup/settings feature-completeness is now closed. Continue with the next unresolved feature-completeness item from current HANDOFF/tree rather than further speculative backup refactors.

Safety/TOCTOU/recovery boundaries remain authoritative.
