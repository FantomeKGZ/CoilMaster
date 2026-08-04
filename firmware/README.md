# Firmware layout

This directory contains independently buildable firmware targets for CoilMaster.

## Targets

- `arduino/` — real-time winding controller for Arduino Uno.
- `esp32/` — Wi-Fi, web portal, RTC, microSD, database and synchronization controller.

## Separation of responsibilities

Arduino Uno owns safety-critical local control: keypad, LCD1602, Hall sensor, SSR, buzzer and the actual winding state machine.

ESP32 owns information services: web UI, profiles, clients, motors, winding history, backups and data exchange over CMP/UART.

The two targets must remain independently buildable and Arduino must keep basic winding capability when ESP32 is unavailable.
