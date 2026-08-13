# Wi-Fi manager, fallback access point and FTP configuration

Status: `PLANNED`  
Target controller: `ESP32`  
Storage: persistent configuration on ESP32 and/or microSD with safe recovery rules.

## Purpose

Add a configurable network subsystem that allows CoilMaster to work in different workshops and networks without recompiling firmware.

The system must support:

- saving several Wi-Fi networks;
- selecting and prioritising known networks;
- editing and deleting saved networks;
- automatic reconnection;
- a local fallback access point when no configured network is available;
- a dedicated Wi-Fi settings page in both desktop and mobile interfaces;
- optional FTP access to the microSD card;
- a dedicated FTP settings page in both desktop and mobile interfaces.

This document describes planned behaviour only. It must not be treated as implemented until firmware, API, UI, tests and documentation are complete.

## Wi-Fi operating modes

The ESP32 network manager must support at least:

```text
STA_CONNECTED
STA_CONNECTING
AP_CONFIGURATION
AP_FALLBACK
NETWORK_DISABLED
```

Recommended behaviour:

1. Load the saved network list.
2. Scan available networks.
3. Try known enabled networks in priority order.
4. Use a bounded connection timeout for each candidate.
5. When a connection succeeds, start normal network services.
6. When no configured network exists or all attempts fail, start the local CoilMaster access point.
7. Keep the configuration web page available through the fallback access point.
8. Never block Arduino real-time control or SSR safety while network connection is unavailable.

## Multiple saved Wi-Fi networks

Each saved network record should contain at least:

```text
id
ssid
password_secret
priority
enabled
hidden_network
use_dhcp
static_ip
 gateway
subnet
dns1
dns2
last_connected_at
last_result
```

The exact storage schema must be versioned before implementation.

Required operations:

- scan nearby networks;
- add a network manually;
- add a hidden network;
- change password;
- enable or disable a saved network;
- reorder priority;
- delete a saved network;
- test connection without immediately destroying the current working configuration;
- show current SSID, IP address, signal level and connection state;
- show the last connection error without exposing the password.

Passwords must never be returned in plain text by API responses or rendered back into HTML. A configured password should be represented only as a masked/unchanged state.

## Fallback access point

The ESP32 must start a local access point when:

- no Wi-Fi networks have been configured;
- all enabled known networks are unavailable;
- credentials are invalid;
- a user explicitly requests configuration mode;
- network configuration is corrupted and cannot be safely loaded.

The fallback AP must provide access to at least:

```text
/
/mobile/settings-wifi.html
/desktop/settings-wifi.html
/api/network/status
/api/network/scan
/api/network/configuration
```

The final route names may change during API design, but mobile and desktop configuration pages are mandatory.

Recommended fallback rules:

- stable AP name derived from the product and device identity;
- non-empty setup password or explicit first-run setup procedure;
- visible warning that the device is in local configuration mode;
- configuration portal remains available even when Internet access does not exist;
- AP mode must not enable SSR or start winding;
- after successful configuration, the user chooses whether to reconnect immediately or reboot safely.

## Wi-Fi settings pages

Create separate pages:

```text
firmware/esp32/web/mobile/settings-wifi.html
firmware/esp32/web/desktop/settings-wifi.html
```

Both pages must support the same capabilities while using layouts appropriate for the device type.

Required UI blocks:

- current network state;
- current IP address and signal strength;
- scan button and scan results;
- saved network list;
- add/edit network form;
- priority and enable/disable controls;
- remove action with confirmation;
- DHCP/static IP selection;
- test connection action;
- fallback AP status;
- clear error and recovery messages.

The normal settings pages should link to the Wi-Fi settings page.

## FTP access to microSD

Add an optional FTP server for maintenance access to the microSD card.

Purpose:

- upload and update website files;
- download logs and backups;
- inspect exported data;
- maintain reference-site content;
- copy approved update packages.

FTP must be disabled by default until credentials and access policy are configured.

Required FTP settings:

```text
enabled
username
password_secret
port
root_directory
read_only
allowed_network_modes
idle_timeout
max_clients
```

Recommended root directory:

```text
/
```

However, the implementation should allow restriction to approved directories such as:

```text
/web
/data/exports
/data/backups
/logs
/updates
```

## FTP safety requirements

FTP access must not compromise machine safety or data integrity.

Mandatory rules:

- FTP can never control SSR or Arduino hardware;
- credentials are never returned in plain text;
- anonymous access is disabled;
- path traversal is rejected;
- concurrent writes to active transactional files are prohibited;
- destructive operations on active journal, pending transaction or job snapshot files must be blocked;
- writing critical data directories during active winding should be denied or switched to read-only mode;
- interrupted uploads use temporary files and atomic rename where possible;
- logs must record login, logout, failed authentication, upload, rename and delete actions;
- configurable idle timeout;
- ability to disable FTP immediately from the web interface;
- FTP must not prevent the local web configuration portal from starting.

Plain FTP does not encrypt credentials or transferred data. Before allowing FTP outside a trusted local network, the project must explicitly decide whether to:

- keep FTP limited to local trusted networks only;
- use a different encrypted transfer method;
- or document and accept the risk.

## FTP settings pages

Create separate pages:

```text
firmware/esp32/web/mobile/settings-ftp.html
firmware/esp32/web/desktop/settings-ftp.html
```

Required UI blocks:

- FTP enabled/disabled state;
- server address and port;
- configured username;
- password change form without showing the existing password;
- access scope/root directory;
- read-only toggle;
- allowed network modes;
- connected client count;
- last connection and last error;
- start/stop action;
- test access/status action;
- security warning for unencrypted FTP.

The normal settings pages should link to the FTP settings page.

## Proposed APIs

Exact names require a separate API contract. Candidate routes:

```text
GET    /api/network/status
GET    /api/network/scan
GET    /api/network/configuration
POST   /api/network/configuration
PUT    /api/network/configuration/{id}
DELETE /api/network/configuration/{id}
POST   /api/network/test
POST   /api/network/reconnect
GET    /api/network/access-point
POST   /api/network/access-point

GET    /api/ftp/status
GET    /api/ftp/configuration
POST   /api/ftp/configuration
POST   /api/ftp/start
POST   /api/ftp/stop
```

Mutating operations should require authentication or a local setup-mode policy before production use.

## Persistent storage and recovery

Configuration must survive restart.

Requirements:

- versioned schema;
- validation before use;
- temporary file plus atomic replacement where supported;
- backup of the last known valid configuration;
- fallback to AP configuration mode when loading fails;
- no silent replacement of invalid credentials with defaults;
- no loss of all saved networks when editing one entry fails;
- password secrets excluded from logs and diagnostics.

## Interaction with existing web variants

Navigation must preserve the active UI variant:

```text
/desktop/settings.html
  -> /desktop/settings-wifi.html
  -> /desktop/settings-ftp.html

/mobile/settings.html
  -> /mobile/settings-wifi.html
  -> /mobile/settings-ftp.html
```

Returning from these pages must return to the matching desktop or mobile settings interface.

## Implementation stages

### Stage 1 — specification and storage

- approve network configuration schema;
- approve credential storage policy;
- implement validation and persistence;
- add unit tests for corrupt and partial configuration.

### Stage 2 — Wi-Fi manager

- scan and prioritise saved networks;
- bounded connection attempts;
- reconnect policy;
- fallback AP;
- status API;
- recovery tests.

### Stage 3 — Wi-Fi UI

- desktop page;
- mobile page;
- navigation from settings;
- masking of secrets;
- connection test and error display.

### Stage 4 — FTP service

- select FTP library/implementation compatible with ESP32 and SD;
- define directory and write-lock policy;
- authentication;
- start/stop lifecycle;
- status API;
- logging and timeout.

### Stage 5 — FTP UI and integration

- desktop page;
- mobile page;
- navigation from settings;
- read-only and directory controls;
- active-winding write restrictions;
- real-device transfer tests.

## Verification scenarios

At minimum test:

1. No saved networks on first boot.
2. One valid saved network.
3. Several networks with different priorities.
4. Highest-priority network unavailable.
5. Wrong password.
6. Hidden network.
7. Static IP validation failure.
8. Configuration file corrupted.
9. Reboot during configuration save.
10. Fallback AP can open mobile and desktop Wi-Fi pages.
11. FTP disabled by default.
12. Wrong FTP password.
13. FTP upload to `/web` and reload of site.
14. Attempted path traversal.
15. Attempted write to protected transactional data.
16. FTP access while winding is active.
17. SD removal during transfer.
18. Power loss during upload.
19. Restart restores valid settings without exposing secrets.

## Status boundary

The following are still not considered implemented:

- per-profile static IP;
- asynchronous scan of nearby networks;
- configurable AP identity/credentials;
- FTP server;
- incoming FTP server credentials and policy;
- recovery FTP upload service.

## Remote router backup checkpoint — 2026-08-13

The target local storage router is a TP-Link TL-WR942N hardware v1. CoilMaster
does not depend on a particular router firmware: it uses a configurable standard
FTP endpoint on the trusted LAN.

Implemented in `4f13a394b2b7467ec71b92022d1f36059c1d6919`:

```text
GET  /api/backup/remote/configuration
POST /api/backup/remote/configuration
POST /api/backup/remote/test
```

The versioned settings contain host, port, username, secret password, remote
directory, enabled state and retention count. Persistence uses verified
`remote-backup.json -> .bak` and `.tmp -> remote-backup.json` replacement with
boot recovery. The API reports only `password_configured`; it never returns the
secret. Desktop and mobile FTP pages can edit the configuration and perform an
FTP greeting/login/CWD test.

The test is fail-closed:

- STA must already be connected to the router;
- `BackupActivityGuard` must prove a safe inactive winding state;
- FTP greeting, login and configured remote directory must all succeed;
- the password is never logged or returned;
- FTP is explicitly documented as trusted-LAN-only because it is unencrypted.

Verified for the checkpoint:

```text
ESP32 Build: SUCCESS
CMP Protocol Tests + web asset/navigation audit: SUCCESS
```

Not yet implemented by this checkpoint:

- actual backup file upload, `.part` verification and atomic rename;
- retention cleanup and scheduling;
- incoming single-client FTP server;
- automatic recovery FTP at `192.168.4.1` when `/web` is absent.

## Bounded AP+STA Wi-Fi checkpoint — 2026-08-13

Implemented in `d7a541acc2f752137783a0b6a0cfb6a86c4d727c`:

```text
GET  /api/network/profiles
POST /api/network/profiles
POST /api/network/profiles/delete
POST /api/network/reconnect
```

The profile store is bounded to five records. Each record contains stable ID,
SSID, secret password, priority, enabled state and hidden-network flag. Secrets
are never returned by API or written to diagnostics. Persistence uses validated
NDJSON plus atomic `.tmp/.bak` replacement and boot recovery.

Runtime now uses non-blocking `WIFI_AP_STA`: the service AP `CoilMaster` remains
at `192.168.4.1`; enabled DHCP profiles are attempted by priority with a
15-second bound; after all fail, AP remains available and retry starts after 30
seconds. No connection wait blocks the web/UART loop and no network operation
starts/resumes winding or controls SSR.

Desktop/mobile pages support add, edit, enable/disable, priority, hidden SSID,
delete, masked password and explicit reconnect.

Verified:

```text
ESP32 Build: SUCCESS
CMP Protocol Tests + web asset/navigation audit: SUCCESS
```

Still requires real-device testing with TL-WR942N v1. Static IP and nearby
network scan remain later stages.
