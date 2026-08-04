# CoilMaster ESP32 firmware

This target will provide the information and network layer of CoilMaster.

## Planned responsibilities

- local Wi-Fi access point/client mode;
- portal and multiple sites stored on microSD;
- desktop and mobile web variants;
- client, motor, repair, winding-history and warehouse databases;
- RTC DS3231 timestamps;
- microSD backups and restore;
- CMP/UART communication with Arduino Uno;
- Hall calibration and equipment diagnostics through controlled APIs;
- receiving winding programs from the web and sending them to Arduino;
- recording a winding as completed only after Arduino confirms the physical result.

## Planned structure

```text
firmware/esp32/
├── src/
├── include/
├── lib/
├── data/          # default web assets copied to microSD/LittleFS
└── test/
```

No ESP32 code may directly bypass Arduino to energize the winding SSR. Remote commands load or control a job through the defined safety workflow.
