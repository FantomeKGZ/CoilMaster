# FTP/Web recovery + network completeness audit — 2026-08-30

Repository: `FantomeKGZ/CoilMaster`  
Working branch: `arduino-ru-lcd-experiment`  
Production/source-of-truth `cmp-protocol-v1` was not modified.

## Exact prior GREEN

Shared Web shell/global-search checkpoint:

```text
CMP Protocol Tests #4590
run 33317957258
head 45a97cf9420e924f16e1385bf89e3eca49d5fa1a
SUCCESS
```

## FTP/Web recovery audit

Confirmed existing safe recovery behavior:

- missing `/web` is detected at boot;
- recovery FTP is restricted to virtual root `/web`;
- `/web` is created when missing;
- FTP accepts clients only from the CoilMaster AP subnet or the connected local STA subnet;
- FTP mutation requires the shared fail-closed `BackupActivityGuard` runtime probe to report `Safe`;
- uploads are written to `.part` and promoted through rename rather than directly overwriting the target;
- the built-in fallback page remains available when the static Web root is unavailable;
- the fallback page exposes the recovery connection information without depending on `/web` assets;
- automatic recovery stops after the Web entrypoints are restored.

### Confirmed defect and fix

The previous completion criteria were inconsistent:

- `StaticSiteServer::storageReady()` requires `/web/index.html`;
- automatic FTP recovery previously considered only `/web/desktop/index.html` and `/web/mobile/index.html`.

Therefore a partially damaged bundle with the root `index.html` missing could show the built-in recovery page while automatic FTP was not guaranteed to be running.

Fixed in:

```text
b7dc361bb85a0497fa33809fbff52025efc7cf8a
```

`WebRecoveryFtpServer::begin()` now independently starts automatic recovery whenever its authoritative Web-entrypoint check is incomplete, and `webRootUsable()` now requires all three:

```text
/web/index.html
/web/desktop/index.html
/web/mobile/index.html
```

No production data root was added to FTP. No backup/restore, UART, SSR, START or winding runtime ownership changed.

Regression coverage was added to the already mandatory final-acceptance audit in:

```text
ae8ba49d5e3cad0f6a593c71b33dc4be31350a1a
```

Exact verification:

```text
ESP32 Build #1787
run 33318283056
head b7dc361bb85a0497fa33809fbff52025efc7cf8a
SUCCESS

CMP Protocol Tests #4592
run 33318325575
head ae8ba49d5e3cad0f6a593c71b33dc4be31350a1a
SUCCESS
```

## Wi-Fi profiles / static IP / network status / coil.local

The requested network feature set is already materially implemented.

Confirmed:

- up to 5 persisted network profiles;
- enabled/disabled and hidden-network flags;
- priority ordering and retry/failover;
- credentials are never returned by the profiles API;
- profile storage uses recoverable temp/main/backup replacement and fails closed on ambiguous corrupted evidence;
- nearby Wi-Fi scan is asynchronous and bounded;
- static IP fields are persisted and server-validated as IPv4;
- static configuration is applied through `WiFi.config()` before `WiFi.begin()`;
- DHCP is explicitly restored for non-static profiles;
- the service AP remains enabled in `WIFI_AP_STA` mode;
- runtime network status exposes AP/STA state, active profile, SSID/IP/RSSI and last connection result;
- `coil.local` is initialized through ESP mDNS and HTTP service registration;
- runtime network status reports whether mDNS is active and the local URL.

### Stale UI source removed

Both Wi-Fi HTML pages still contained old text claiming that static IP would be added later, even though `shared/settings-wifi.js` already injects the complete static-IP UI and backend support exists.

Removed obsolete text only:

```text
34ce942293352e393078f96472345b962ef3f333  desktop
9ea9d9e9995384685bf2814f6378559d55b23afb  mobile
```

No Wi-Fi behavior changed in those commits.

These HTML commits and this documentation commit are newer than the exact GREEN heads above. Do not call the current branch head GREEN until its own exact workflow reports SUCCESS.

## Next audit target

Continue final completeness audit with:

1. backup/settings workflows;
2. full remaining desktop/mobile feature parity;
3. stale/empty pages;
4. dead links/settings links;
5. remaining previously promised functions only where current source proves an actual gap.

No new hardware test is required for these Web/recovery/settings audit changes. A physical test is still reserved for a concrete firmware runtime regression or final production acceptance.
