# Checkpoint 20 — client creation + shared shell + FTP/Web recovery + Wi-Fi audit

Date: 2026-09-03
Branch: `arduino-ru-lcd-experiment`
Production `cmp-protocol-v1` remains untouched.

## Client creation form — CLOSED

Backend `POST /api/clients` remains authoritative and unchanged:

- required: `name`;
- required: `phone` with at least 7 normalized digits;
- optional: `comment`.

Desktop/mobile `client-new.html` now clearly separate required and optional data, trim values, validate phone before submit, preserve the single-flight duplicate-submit guard, and do not create a repair automatically. Mobile opens the created client card. Desktop retains the existing canonical handoff through `/desktop/repairs.html?client_id=...` for starting a repair.

Regression contract: `Tests/Web/check_client_new_ui.js`, executed through `Tests/Web/check_web_assets.js`.

Exact recovery evidence:

```text
7c63d6838f715471751634f66ce296ea7c4b9228
CMP Protocol Tests #4781 run 33718002444 / SUCCESS

f2ba232e63bf093e92df3d19d09770959cb23a55
CMP Protocol Tests #4782 run 33718025915 / SUCCESS
```

Earlier #4777/#4778/#4779/#4780 failures are superseded by the exact successful recovery runs above; do not treat them as current failures.

## Shared Web shell audit — NO-CHANGE

Current shared shell and its CMP contract already cover:

- canonical desktop/mobile navigation;
- global motor/client/repair search;
- breadcrumbs;
- bounded recent items;
- device clock;
- firmware/web provenance/version;
- toast feedback;
- desktop/mobile route parity;
- read-only shell safety.

`Tests/Web/check_shared_app_shell_contracts.js` is already executed by CMP and was successful on the exact #4782 checkpoint. No runtime change was justified.

## FTP/Web recovery — CLOSED

Runtime was already implemented correctly:

- FTP is isolated to `/web`;
- recovery FTP can start automatically when `/web` is absent/unusable;
- normal manual START/STOP is operator-controlled and safe-idle gated;
- upload uses a `.part` staging file followed by rename to the final path;
- path traversal/out-of-root access is rejected;
- local-network access policy remains enforced;
- workshop/process data outside `/web` is not exposed by FTP.

A real CI coverage gap was found: `Tests/Web/check_web_recovery_ftp_contracts.js` existed but was not executed by CMP. It is now required from `Tests/Web/check_web_assets.js`.

Exact GREEN:

```text
8a08140ea680ab2d4a125f4bc15d778011767d91
CMP Protocol Tests #4783 run 33718397074 / SUCCESS
```

## Wi-Fi profiles / static IP / coil.local — NO-CHANGE

Current runtime already implements the requested network behavior:

- multiple persisted Wi-Fi profiles;
- deterministic priority ordering with profile fallback;
- enabled/hidden profile metadata;
- DHCP mode;
- static local IP, gateway, subnet, optional DNS1/DNS2;
- IPv4 validation;
- recoverable atomic profile-file replacement;
- AP+STA recovery behavior;
- local mDNS hostname `coil`;
- HTTP service advertised by mDNS, giving `http://coil.local/` when mDNS is active;
- UI only presents the hostname as active when runtime mDNS status says it is active, otherwise it tells the operator to use the IP address.

Existing CMP contracts already protect this:

```text
Tests/Web/check_network_profile_atomic_recovery.js
Tests/Web/check_network_json_escaping.js
```

Both are executed by CMP and passed on exact checkpoint #4783. No network runtime modification is justified.

## Immediate NEXT

Continue feature-completeness audit with:

1. backup/settings;
2. desktop/mobile feature parity;
3. stale/empty pages and links;
4. any other previously promised feature found incomplete.

For every proven gap: fetch exact current branch file + blob SHA, make the minimum compatible change, add/extend regression coverage, verify exact CI, and then update HANDOFF. Production remains untouched.
