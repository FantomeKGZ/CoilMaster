# CoilMaster build information

## Source of truth

```text
Repository: FantomeKGZ/CoilMaster
Branch: cmp-protocol-v1
```

`main` не используется как источник прошивки.

## Targets

```text
pio run -e uno
pio run -e esp32
```

- Arduino Uno: production entry point `firmware/arduino/src/main.cpp`.
- ESP32 DevKit V1: production entry point `firmware/esp32/src/main.cpp`.
- ESP32 partition: `huge_app.csv`, physical flash 4 MB, PSRAM отсутствует.

## Verification policy

Build или CI считается подтверждённым только после фактического `SUCCESS` для
соответствующего commit. Старые RAM/Flash значения нельзя переносить на новый
commit.

Текущий проверенный статус, hardware checkpoints и процент готовности всегда
записываются в:

```text
docs/PROJECT_HANDOFF/01_CURRENT_STATE.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
```

## Safety invariants

- physical START только физический;
- auto-resume после reboot отсутствует;
- ESP32/Web не управляют SSR напрямую;
- `RUN_COMPLETED` не списывает провод;
- wire writeoff ручной и требует exact `spool_id + source_session_id + source_run_id`.
