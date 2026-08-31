# Wi-Fi profiles, static IP and coil.local — 2026-08-31

Branch: `arduino-ru-lcd-experiment`.
Production `cmp-protocol-v1` was not modified.

## Audit result

The network backend was already substantially complete:

- up to 5 persisted Wi-Fi profiles;
- priority ordering and enabled/hidden flags;
- non-blocking AP+STA manager with recovery AP `CoilMaster` at `192.168.4.1`;
- DHCP and validated static IPv4 (`local_ip`, `gateway`, `subnet`, optional DNS1/DNS2);
- atomic `.tmp/.bak` profile persistence recovery;
- bounded async nearby-network scan;
- password preservation on edit and no credential exposure in GET responses;
- explicit reconnect;
- mDNS backend using hostname `coil`, advertised as HTTP on port 80;
- `/api/system/network` already exposed `mdns_active`, `local_hostname` and `local_url`.

The proven completeness gap was UI-only: `settings-wifi.html` did not show the already-working `coil.local` address to the operator.

## Fix

Shared controller `firmware/esp32/web/shared/settings-wifi.js` now adds a `Локальный адрес` row to the runtime network card.

When backend reports `mdns_active=true`, the row shows a clickable runtime-provided `local_url` / `local_hostname` (normally `http://coil.local/`).

When mDNS is inactive, the UI explicitly reports `недоступен; используйте IP` instead of promising a hostname that did not start.

Because desktop and mobile Wi-Fi pages use the same shared controller, both variants receive the same behavior without duplicating HTML.

Implementation:

```text
410190ffb44e0dd84b75f998ffd72f858de70cec
```

Regression:

```text
f68a7d994eced5dfbbfa003025fb857daeab2529
```

The regression now locks:
- `MDNS.begin("coil")` ownership and HTTP service advertisement;
- runtime mDNS status propagation;
- `local_hostname` / `local_url` network API contract;
- UI rendering only when mDNS is actually active;
- static IPv4/DNS validation and application;
- DHCP reset path;
- profile priority sorting;
- desktop/mobile use of the shared Wi-Fi controller;
- existing JSON escaping and single-flight save semantics.

## Exact verification

Implementation SHA `410190ffb44e0dd84b75f998ffd72f858de70cec`:

```text
CMP #4731 / run 33368887040 / SUCCESS
ESP32 #1844 / run 33368887045 / build job SUCCESS
Arduino RU LCD #275 / run 33368887075 / compare-builds job SUCCESS
```

Regression SHA `f68a7d994eced5dfbbfa003025fb857daeab2529`:

```text
CMP #4732 / run 33368927775 / SUCCESS
```

## Safety

No C++ firmware behavior was changed in this block. Physical START, SSR ownership, reboot fail-closed behavior, RUN_WIRE accounting and backup/restore safety invariants are unchanged.

## Next

Continue feature-completeness audit with backup/settings, then desktop/mobile parity and stale/empty page/link checks. Change only proven incomplete behavior.
