# Checkpoint 168 — FTP/Web recovery and backup/settings parity

Date: **2026-08-31**  
Branch: **`arduino-ru-lcd-experiment`**  
Production/source-of-truth: **`cmp-protocol-v1`** — unchanged.

## Result

FTP/Web recovery plus backup/settings feature-completeness is closed as **NO-CHANGE runtime**. No backend/production behavior change is required.

Confirmed runtime properties:

- desktop/mobile FTP settings use one shared controller;
- operator FTP controls use `/api/ftp/status` and explicit POST start/stop actions;
- FTP server is restricted to `/web`;
- upload replacement uses `.part` before final rename;
- FTP mutation is guarded by activity-safe state and local-network checks;
- incomplete/missing required Web entrypoints enable recovery FTP, and recovery stops when `/web` is usable;
- remote backup settings have desktop/mobile parity for host, port, username, remote directory, enable, retention and schedule;
- retention stays bounded to `1..30`;
- schedule fields are shared and validated;
- blank password preserves the stored credential and API responses do not expose credentials;
- no physical START, SSR, RUN evidence, Warehouse or wire-writeoff semantics changed.

## Restore UI boundary

The static backup pages intentionally remain read-only:

```text
static/offline desktop/mobile backup.html
    -> backup export/read-only page
    -> no direct backup-remote-upload.js include

live ESP32-served /backup.html
    -> CM_StaticSiteServer injects /shared/backup-remote-upload.js
    -> full operator backup/restore controls
```

The live operator sequence remains explicit and fail-closed:

```text
inspection
-> staging
-> restore plan
-> rollback snapshot
-> apply preflight
-> typed APPLY confirmation
-> apply
```

Discard remains explicit. No automatic restore/resume is introduced.

## Regression locks

```text
Tests/Web/check_web_recovery_ftp_contracts.js
Tests/Web/check_remote_backup_ui_parity.js
Tests/Web/check_ci_trigger_contracts.js
```

The contracts lock FTP recovery, live/offline restore-helper boundary, route parity, staging discard, typed `APPLY`, stale guard behavior, settings parity, retention, schedule and credential handling.

## Exact verification history

### FTP recovery contract

```text
226cc65a82cfc42bc172fe27b53153e669062812
CMP #4740 / run 33371841283 / SUCCESS

d0ff4b0cd4fc5197511012f6baa4dd033caa2ac7
CMP #4741 / run 33371920243 / FAILURE
reason: regression incorrectly expected literal /api/ftp/start while the shared controller correctly builds /api/ftp/ + action

5f603a8edd17a10e69cb95fa7f4767a5776396b1
docs(handoff): record FTP web recovery audit
CMP #4742 / run 33372086838 / FAILURE
reason: docs commit inherited the still-failing regression from #4741; no new runtime defect

8a3eadd260b9c1d33c9cf44e5fbd203e261b7e69
CMP #4743 / run 33372151251 / SUCCESS

fadda308bfe0996d20f3b20c7984d662e51a90c8
docs(handoff): correct FTP recovery verification chain
CMP #4744 / run 33372329213 / SUCCESS
```

### Temporary direct static restore includes — superseded

These commits were CI-successful but were deliberately reverted after the existing runtime-injection architecture was confirmed. They are historical only and must not be treated as the final restore UI design.

```text
f75b5651c1f8f4060af590c471b67e0d524f8891
fix(web): expose remote backup restore controls
CMP #4745 / run 33372451983 / SUCCESS
ESP32 #1847 / run 33372451988 / SUCCESS
Arduino RU LCD #278 / run 33372452027 / SUCCESS

5340da86d49620cd1a397d3de49ac6404d486c3e
fix(web): expose mobile restore controls
CMP #4746 / run 33372503696 / SUCCESS
```

### Final restored runtime-injection boundary

```text
c6c54cd81a23f3bae582a5cee25ece4b1ef69275
revert(web): preserve runtime restore injection boundary
CMP #4747 / run 33372564846 / SUCCESS

75b36c9875fd1fd271b433e8db4b940d43835fea
revert(web): preserve mobile runtime restore injection boundary
CMP #4748 / run 33372615230 / SUCCESS
ESP32 #1850 / run 33372615249 / SUCCESS
Arduino RU LCD #281 / run 33372615235 / SUCCESS
```

This `75b36c98...` triple is the strongest exact firmware/runtime evidence for the final intended restore boundary.

### Remote-backup parity contract

```text
8eced27f043140744cdb6c630c040f5d876f8250
CMP #4749 / run 33372872066 / SUCCESS

ab0c1cbcf95091561d4b507bfbf383889ed4502b
CMP #4750 / run 33372914424 / FAILURE
reason: regression matched the escaped C++ JSON credential-hidden literal too strictly; runtime still returned credentials_exposed=false

fbd292f086cf77a01825095c32676ac461f88f9c
CMP #4751 / run 33373033715 / SUCCESS
```

### Checkpoint documentation HEAD before this update

```text
e0968ea525e12b075798671424f409d14c07cf7a
docs(handoff): close backup settings parity audit
CMP #4752 / run 33373131944 / SUCCESS
```

`#4741`, `#4742` and `#4750` are intermediate regression/test-contract failures, not final runtime failures. `#4745` and `#4746` are successful but superseded temporary UI commits.

## Next mandatory gate

Repo-reviewable software work is closed for this block. Do not start another speculative optimization.

Next mandatory validation is the physical two-board Arduino Uno + ESP32 end-to-end acceptance flow. Use `16_PHYSICAL_E2E_ACCEPTANCE_2026-08-31.md` as the operator checklist once created.

Safety/TOCTOU/recovery boundaries remain authoritative. Production remains unchanged.
