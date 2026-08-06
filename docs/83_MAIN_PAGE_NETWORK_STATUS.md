# Network status on the main CoilMaster dashboards

Status: `PLANNED`  
Target: ESP32 firmware, status API, desktop home page and mobile home page.

## Purpose

The current network state and the address used to open CoilMaster must be visible directly on the main page. The operator should not need to open the Wi-Fi settings page just to find the ESP32 address or understand how it is connected.

This requirement extends:

```text
docs/82_WIFI_MANAGER_AND_FTP_CONFIGURATION.md
```

It remains planned until firmware, API, desktop UI, mobile UI and verification are complete.

## Required main-page information

Both main interfaces must display a network status block:

```text
firmware/esp32/web/desktop/index.html
firmware/esp32/web/mobile/index.html
```

The block must show at least:

- current network state;
- current operating mode;
- connected Wi-Fi SSID when ESP32 works as a station;
- ESP32 IP address currently usable by the operator;
- local access-point name when AP mode is active;
- access-point IP address when AP mode is active;
- signal strength when connected to an external Wi-Fi network;
- connection error or fallback reason;
- FTP enabled/disabled status when the FTP service is implemented;
- a direct link to the matching Wi-Fi settings page.

## Network states shown to the user

Recommended user-facing states:

```text
Подключено к Wi-Fi
Подключение к Wi-Fi
Локальная точка доступа
Режим настройки
Сеть отключена
Ошибка сети
```

Internal/API states may remain machine-readable, but the UI must translate them into clear Russian labels.

## Current address

The most important displayed value is the address at which the current browser or another device can open CoilMaster.

Examples:

```text
Wi-Fi: http://192.168.1.47/
Точка доступа: http://192.168.4.1/
```

The address must come from the ESP32 network subsystem. It must not be hard-coded as `192.168.4.1`, because station mode, static IP settings and future AP configuration can change the actual address.

When both STA and AP interfaces are active, the API should return both addresses and identify the preferred/current operator address.

## Proposed status fields

The existing status endpoint may be extended, or a dedicated network endpoint may be used. The preferred first implementation is to keep a compact network summary in:

```text
GET /api/status
```

Recommended fields:

```json
{
  "network_state": "STA_CONNECTED",
  "network_mode": "STA",
  "network_connected": true,
  "network_ssid": "WorkshopWiFi",
  "network_rssi_dbm": -58,
  "network_signal_percent": 82,
  "network_ip": "192.168.1.47",
  "network_url": "http://192.168.1.47/",
  "ap_active": false,
  "ap_ssid": "CoilMaster-1234",
  "ap_ip": "192.168.4.1",
  "fallback_reason": "",
  "ftp_enabled": false,
  "ftp_running": false
}
```

Secrets must never be included. In particular, the API must never return Wi-Fi or FTP passwords.

A future detailed endpoint may remain:

```text
GET /api/network/status
```

The main page should use only the summary fields required for frequent refresh.

## Desktop layout

The desktop dashboard should add a visible card or system-health section with:

- status badge;
- SSID or `Локальная точка доступа`;
- current IP address;
- clickable current URL;
- signal quality;
- Wi-Fi settings link;
- FTP state when available.

The address should remain selectable and copyable. It should not be hidden only inside a tooltip.

## Mobile layout

The mobile dashboard should show a compact network card with:

- connection icon and state;
- SSID/AP name;
- current IP address;
- tap-to-open or tap-to-copy address;
- link to `/mobile/settings-wifi.html`;
- concise warning when fallback AP is active.

The block must fit the existing mobile card layout and must not replace winding safety information.

## Refresh behaviour

Network summary should refresh together with the existing status polling.

Requirements:

- no full-page reload;
- display `Нет данных` rather than stale values after repeated API failure;
- show transitions between STA and AP modes;
- update the IP address after reconnect;
- do not expose saved credentials;
- avoid excessive Wi-Fi scans from the main page;
- main-page polling reads status only and must not start a scan.

## Fallback AP indication

When fallback AP is active, the main page must clearly say why, for example:

```text
Локальная точка доступа
Причина: сохранённые сети недоступны
Адрес: http://192.168.4.1/
```

The page must provide a direct action:

```text
Настроить Wi-Fi
```

Desktop route:

```text
/desktop/settings-wifi.html
```

Mobile route:

```text
/mobile/settings-wifi.html
```

## Safety and independence

Network display and network failures must not affect real-time winding control.

Mandatory rules:

- loss of Wi-Fi does not command SSR;
- loss of the browser does not stop Arduino safety logic;
- AP/STA transitions do not automatically start or continue winding;
- network information is informational and service-level only;
- API polling must remain lightweight enough not to interfere with UART and SD operations.

## Implementation stages

1. Define canonical network state and mode enums/strings.
2. Implement a network-status snapshot in the ESP32 network manager.
3. Add summary fields to `/api/status`.
4. Add desktop network card.
5. Add mobile network card.
6. Add links to the matching Wi-Fi settings pages.
7. Add FTP state after the FTP service exists.
8. Verify mode and IP transitions on real hardware.

## Verification scenarios

At minimum test:

1. ESP32 connected to a saved Wi-Fi network by DHCP.
2. ESP32 connected with a configured static IP.
3. No saved networks; fallback AP starts.
4. Saved networks unavailable; fallback AP starts.
5. Wi-Fi reconnect changes the IP address.
6. STA and AP are active simultaneously.
7. Main page opens through the AP address.
8. Main page opens through the station address.
9. Status API fails temporarily and stale address is not presented as current indefinitely.
10. Passwords are absent from all status responses.
11. Desktop and mobile pages show matching network facts.
12. Winding and UART behaviour remain unaffected during reconnect.

## Status boundary

The current fixed access point and the currently hard-coded service address do not satisfy this requirement. Completion requires live firmware-derived status and IP data on both main pages.
