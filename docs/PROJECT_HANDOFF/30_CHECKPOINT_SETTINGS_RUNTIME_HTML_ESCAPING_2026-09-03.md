# Checkpoint 30 — Settings runtime HTML escaping — 2026-09-03

Branch: `cmp-protocol-v1`

## Scope

Continuation of checkpoint 29 runtime/DOM boundary audit. Only demonstrated presentation-layer defects were changed. No machine-control, calibration ownership, backup safety or persistence semantics were changed.

Rule enforced in this checkpoint:

- dynamic runtime strings interpolated into `innerHTML` must be HTML-escaped first;
- values assigned with `.textContent` or form-control `.value` do not require HTML escaping;
- backend JSON escaping remains transport protection and is not a substitute for HTML escaping in the browser.

## 1. Time / RTC status

Affected pages:

- `firmware/esp32/web/desktop/settings-time.html`
- `firmware/esp32/web/mobile/settings-time.html`

Confirmed gap:

- `local_time`;
- `timezone`;
- unknown/fallback `ntp_status`

were interpolated directly into the status block `innerHTML`.

Fix commits:

```text
41957d4e4ca064fa35c4ebab4256cdd8e37ded22  fix(web): escape desktop time status
aa36c3bc2bdf55f5dff8b9a6ee7521ec82a2260a  fix(web): escape mobile time status
725013ee7346df903d488e3630f6a948709dc4d2  test(web): protect time status escaping
```

Exact regression verification:

```text
CMP #4868
run 33734667118
head 725013ee7346df903d488e3630f6a948709dc4d2
completed/success
```

## 2. Hall settings state

Affected pages:

- `firmware/esp32/web/desktop/settings-hall.html`
- `firmware/esp32/web/mobile/settings-hall.html`

Confirmed gap:

- `source`;
- `last_reply`

from `/api/hardware/hall` were interpolated directly into `stateBox.innerHTML`.

Telemetry values were already rendered through `textContent` and were left unchanged.

Fix commits:

```text
083828a862d5a35a56ef0fd7b2474dce63cdc558  fix(web): escape desktop hall state
cc1af7388a9f9241bf77f2537cc8e07375ebd0e8  fix(web): escape mobile hall state
e522ccee8d8e6a5b958348ec6abf8bd4acde0c44  test(web): protect Hall state escaping
```

Exact regression verification:

```text
CMP #4871
run 33734862776
head e522ccee8d8e6a5b958348ec6abf8bd4acde0c44
completed/success
```

No Hall calibration/start/SSR behavior changed. Physical START remains mandatory and Arduino remains the sole SSR owner.

## 3. FTP / Web recovery runtime status

Affected shared UI:

- `firmware/esp32/web/shared/settings-remote-backup.js`

Both desktop and mobile FTP pages use this shared script.

Confirmed gap:

- AP/STA FTP address strings;
- FTP port presentation;
- unknown/fallback `last_result`

were interpolated into `device.innerHTML` without explicit HTML escaping.

Fix commits:

```text
205d2353acd3d0aa6aa92e8eb49a822ace3608ab  fix(web): escape FTP runtime status
93d9b3cc74d3743cf95f460472a06b0d7298895b  test(web): protect FTP status escaping
```

Exact runtime verification for `205d2353...`:

```text
CMP #4872       run 33734956028 / SUCCESS
Reference #126  run 33734956275 / SUCCESS
ESP32 #1890     run 33734955992 / SUCCESS
```

Exact final regression verification:

```text
CMP #4873
run 33734985565
head 93d9b3cc74d3743cf95f460472a06b0d7298895b
completed/success
```

FTP start/stop safety gating, `/web`-only scope and active-winding restrictions were not changed.

## Regression coverage

`Tests/Web/check_settings_hub_parity.js` now protects runtime HTML escaping for:

- settings network summary;
- time / RTC status;
- Hall state;
- FTP / Web recovery runtime status.

## Audited NO-CHANGE blocks

### Service job

Desktop/mobile `service-job.html` already use `textContent` for runtime job/session/status/program/context and preserve strict cancel/dismiss state restrictions.

### Wi-Fi settings

`firmware/esp32/web/shared/settings-wifi.js` already HTML-escapes SSID/IP/runtime strings before `innerHTML` and uses `textContent` for status/error text.

### Backup export

Desktop/mobile `backup.html` already escape dynamic file/session/error values where HTML is constructed and use `textContent` elsewhere. Export/restore safety boundaries remain unchanged.

## Safety invariants preserved

- no automatic physical START;
- no automatic repeat START;
- no auto-resume after reboot;
- Arduino remains the sole SSR owner;
- ESP32/Web does not directly control SSR;
- `RUN_COMPLETED` does not auto-writeoff wire;
- manual RUN_WIRE exact provenance remains unchanged;
- backup/restore remains operator-controlled and fail-closed;
- FTP recovery remains constrained to `/web` and safe runtime states.

## Current implementation head before this documentation commit

```text
93d9b3cc74d3743cf95f460472a06b0d7298895b
```

Its exact applicable final regression is `CMP #4873 / run 33734985565 / completed/success`.

This documentation commit requires its own exact CI result before the documentation HEAD itself may be called GREEN.
