# Checkpoint — compact mobile Arduino task list

Date: 2026-08-31
Branch: `arduino-ru-lcd-experiment`
Production `cmp-protocol-v1` unchanged.

## Goal

Make the mobile Arduino archive show the same task set as desktop, but in a shorter operator-friendly list.

## Result

Mobile `Задачи Arduino` now renders each task as a compact row/card with:

- short `#RUN`;
- winding program shown prominently as turns, e.g. `38/38 витков`;
- localized type: `Рабочая` / `Пусковая`;
- completion status;
- linkage summary (`Двигатель №...` or `Не привязано`).

The following remain available under `Подробнее` instead of occupying permanent vertical space:

- exact `Session / Run`;
- completed/planned repeats;
- historical RUN count;
- RUN evidence description;
- exact backend type/assignment role/full motor label.

Desktop archive rendering is unchanged.

## Files

- `firmware/esp32/web/shared/arduino-windings-archive.js`
- `firmware/esp32/web/mobile/arduino-windings.html`
- `Tests/Web/check_arduino_archive_ui.js`

## Safety / data semantics

This is Web-only display work. No backend archive format, UART protocol, physical START, SSR ownership, automatic resume, RUN evidence, linkage evidence, or material write-off semantics changed.

## Commits

- `f6d0d4a0011870a70064c60c69c62c100d74cc3e` — compact mobile task renderer
- `bfd3b6a38f9246132f05a0eb16328392a2a33397` — mobile compact styling/text
- `2a119a47a77d597ba9303b578a54c4bcf031116d` — regression guard

## Exact CI

`CMP Protocol Tests #4672`, run `33360677905`, head `2a119a47a77d597ba9303b578a54c4bcf031116d`: `completed/success`.

This GREEN evidence applies to that exact test HEAD. Any newer documentation commit needs its own exact SUCCESS before being called GREEN.

## Deployment

Only `/web` needs updating for this feature. No ESP32 or Arduino firmware flash is required.
