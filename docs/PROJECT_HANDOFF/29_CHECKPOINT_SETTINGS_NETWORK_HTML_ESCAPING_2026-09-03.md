# Checkpoint 29 — settings network summary HTML escaping

Date: 2026-09-03

## Scope

Closed a concrete Web rendering/security defect in desktop/mobile settings on active branch `cmp-protocol-v1`.

## Proven defect

Both settings hubs read runtime network state from:

```text
GET /api/system/network
```

The backend already JSON-escapes string fields correctly. However the settings pages then interpolated runtime values directly into `networkSummary.innerHTML`.

Affected dynamic fields included:

```text
mode
ap_ssid
ap_ip
sta_ssid
sta_ip
sta_rssi (desktop)
```

JSON escaping protects the transport syntax only. It does not make an external string safe for HTML insertion. In particular SSID values may contain HTML-significant characters such as `<`, `>`, `&`, quotes or apostrophes.

## Runtime fix

Updated:

```text
firmware/esp32/web/desktop/settings.html
firmware/esp32/web/mobile/settings.html
```

Both pages now use an explicit HTML escaping helper before runtime network strings are concatenated into `networkSummary.innerHTML`.

Desktop commit:

```text
4a3a8735414a56c1322f36338862940ae3df9019
fix(web): escape settings network summary
```

Mobile commit:

```text
5622ed08465291dc8708aba4285be735d421e6f5
fix(web): escape mobile network summary
```

No backend network semantics, Wi-Fi state, credentials, reconnect behavior, FTP state or persistence were changed.

## Regression protection

Extended existing:

```text
Tests/Web/check_settings_hub_parity.js
```

It now requires desktop/mobile settings to HTML-escape runtime network strings before `innerHTML` rendering, including:

```text
mode
AP SSID/IP
STA SSID/IP
STA RSSI on desktop
```

Regression commit:

```text
ebede167b5fd62277875647e669e907c8fe4dad4
test(web): protect settings network escaping
```

The existing `check_network_json_escaping.js` remains responsible for the separate backend JSON-transport contract. The two layers are intentionally independent:

```text
backend JSON escaping
+
frontend HTML escaping
```

## Exact CI evidence

Desktop runtime commit `4a3a873...`:

```text
CMP Protocol Tests #4862
run 33734113796
completed/success

Reference Legacy Import Check #120
run 33734113878
completed/success

ESP32 Build #1884
run 33734113844
completed/success
```

Mobile runtime commit `5622ed08...`:

```text
CMP Protocol Tests #4863
run 33734170001
completed/success

Reference Legacy Import Check #121
run 33734170066
completed/success

ESP32 Build #1885
run 33734170099
completed/success
```

Final regression HEAD `ebede167...`:

```text
CMP Protocol Tests #4864
run 33734195479
completed/success
```

## Safety invariants unchanged

No physical START, SSR, UART, job, warehouse, writeoff, costing, repair finalization, backup or restore behavior changed.

Still enforced:

- physical START only;
- no automatic repeat START;
- no auto-resume after reboot;
- Arduino is sole SSR owner;
- ESP32/Web never directly controls SSR;
- `RUN_COMPLETED` alone does not write off wire;
- manual RUN_WIRE retains exact spool/session/run provenance;
- restore remains operator-controlled and fail-closed.

## Next step

Continue the final feature/runtime acceptance audit. Change code only for another concrete current defect; otherwise record the reviewed area as NO-CHANGE.