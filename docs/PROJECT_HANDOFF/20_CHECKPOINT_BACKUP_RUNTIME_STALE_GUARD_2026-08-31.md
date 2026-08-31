# Checkpoint — Backup runtime STALE guard and settings parity

Date: **2026-08-31**  
Branch: **`arduino-ru-lcd-experiment`**  
Production/source-of-truth: **`cmp-protocol-v1` — unchanged**

## Defect closed

The static desktop/mobile backup pages execute `/shared/backup-restore-stale-guard.js` before the live ESP32 `CM_StaticSiteServer` later injects `/shared/backup-remote-upload.js` for `/backup.html`.

An earlier optimization treated the initial absence of restore controls as proof that the page would remain read-only. That was correct for the offline/static bundle but incorrect for the live ESP32 page: the guard could exit before runtime restore/apply controls appeared, leaving those later controls without the intended fail-closed `apply-status` STALE evidence check.

## Fix

- the static/offline backup page remains read-only and performs no `/api/backup/remote/apply-status` HTTP polling while restore UI is absent;
- the shared stale guard waits for delayed runtime controls using a DOM `MutationObserver` only;
- once live remote-backup controls appear, that presence observer disconnects and the existing fail-closed STALE guard starts;
- `STALE` still disables unsafe backup restore/apply controls while stage discard remains available for operator recovery;
- no restore backend semantics were changed;
- no auto-resume, auto-apply or automatic production mutation was introduced.

## Backup/settings parity audit

The surrounding operator surface was audited against the real branch tree:

- desktop and mobile live backup pages receive the same `/shared/backup-remote-upload.js` runtime helper through `CM_StaticSiteServer`;
- the shared remote-backup UI exposes full backup/retention, list/inspect, stage, restore-plan, rollback snapshot, apply-preflight, typed-ID apply and status flows;
- apply remains operator-controlled and explicitly confirmed;
- desktop and mobile `settings-ftp.html` both load `/shared/settings-remote-backup.js`;
- remote host/port/path/credentials, retention, batch and schedule settings are reachable from both UI variants;
- FTP start/stop remains separately operator-controlled and the FTP recovery surface remains restricted to `/web`;
- no separate desktop/mobile parity defect was found in this audit.

## Commits and CI evidence

```text
d25cf6d5c417febf819d5c173d1397bf9f6b7245  fix(web): arm restore stale guard after runtime UI injection
CMP Protocol Tests #4736  run 33370488062 / SUCCESS

97a3c1bf6773488a60328118777fb34370dd66a5  first delayed-UI regression attempt
CMP Protocol Tests #4737  run 33370535928 / FAILURE
Reason: regression source-text assertion incorrectly rejected the valid wait-loop line inside the MutationObserver; runtime behavior itself was not the failing condition.

66fc155dd03e28da0bab1d244112c93324c660d6  test(web): distinguish delayed wait from permanent stale-guard exit
CMP Protocol Tests #4738  run 33370704038 / SUCCESS
```

Earlier commits `e25a5c57820c4348716ee7aac18ff96d470f9662` / `69bc362b16120554db736db313ef9fa745f57e5c` and their GREEN runs `#4734/#4735` proved the no-polling optimization and its first regression contract, but did not model the later runtime UI injection. This checkpoint supersedes that incomplete assumption while retaining the intended no-polling behavior for static/offline backup pages.

## Result

Backup/settings feature-completeness audit is closed for this defect. The current safe contract is: **no unnecessary backend polling while the page is genuinely read-only; fail-closed STALE monitoring automatically arms when the live ESP32 injects restore controls.**
