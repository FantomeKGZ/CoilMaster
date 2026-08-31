# Checkpoint 168 — FTP/Web recovery audit

Date: **2026-08-31**  
Branch: **`arduino-ru-lcd-experiment`**  
Production/source-of-truth: **`cmp-protocol-v1`** — unchanged.

## Result

FTP/Web recovery runtime audit is **NO-CHANGE**. No backend/runtime defect requiring a behavioral change was found.

The audit confirmed:

- desktop and mobile FTP settings use the same shared controller;
- operator controls use `/api/ftp/status`, `/api/ftp/start`, `/api/ftp/stop`;
- remote-backup configuration/test on the same page uses `/api/backup/remote/configuration` and `/api/backup/remote/test`;
- FTP storage remains confined to `/web`;
- upload replacement uses a temporary `.part` file before rename to the final path;
- mutation requests remain guarded by the runtime activity safety probe;
- local-network access remains restricted by the server-side subnet check;
- automatic recovery is activated when the required web entrypoints are absent and stops when `/web` becomes usable;
- web readiness requires root, desktop and mobile entrypoints;
- no physical START, SSR, RUN evidence, Warehouse, wire-writeoff or backup-restore mutation semantics were changed.

## Regression lock

A dedicated repository contract now protects the audited behavior:

```text
Tests/Web/check_web_recovery_ftp_contracts.js
```

It is invoked from the existing mandatory Web/CI contract audit:

```text
Tests/Web/check_ci_trigger_contracts.js
```

Commits:

```text
ca8ab7a70e22c2ec4ae38c5654ef25e0cc57b627  test(web): lock FTP recovery contracts
d0ff4b0cd4fc5197511012f6baa4dd033caa2ac7  test(web): run FTP recovery contract audit
```

Exact code/test HEAD before this documentation commit:

```text
d0ff4b0cd4fc5197511012f6baa4dd033caa2ac7
```

CI for this code/test HEAD must be recorded only after an exact `completed/success` run is independently confirmed. Do not transfer GREEN from earlier commits.

## Next repo-reviewable block

Continue the remaining backup/settings feature-completeness audit:

1. remote-backup/restore UI ↔ backend route parity;
2. retention/batch/inspect controls;
3. stage → restore-plan → rollback snapshot → apply-preflight → apply sequencing;
4. discard/recovery availability;
5. desktop/mobile reachability/parity;
6. stale-guard and fail-closed mutation behavior.

Only confirmed defects should change runtime code. Safety/TOCTOU/recovery boundaries remain authoritative.
