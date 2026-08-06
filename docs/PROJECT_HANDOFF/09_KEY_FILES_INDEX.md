# Индекс ключевых файлов

Перед редактированием всегда получать актуальное содержимое файла из ветки `cmp-protocol-v1`.

## UART и намотка ESP32

```text
firmware/esp32/src/CM_UartEventReceiver.h
firmware/esp32/src/CM_UartEventReceiver.cpp
firmware/esp32/src/CM_WindingJournal.h
firmware/esp32/src/CM_WindingJournal.cpp
```

Назначение:

- приём UART-событий;
- передача заданий;
- ACK/REJECT/TIMEOUT/CANCEL;
- повторные отправки;
- строгий парсинг;
- журнал `RUN_STARTED` и `RUN_COMPLETED`;
- защита последовательности событий.

## Склад провода

Основные файлы находятся среди:

```text
firmware/esp32/src/CM_WarehouseStore.h
firmware/esp32/src/CM_WarehouseWriteOffHistory.cpp
firmware/esp32/src/CM_WarehouseWriteOffWeb.cpp
firmware/esp32/src/CM_WarehouseMaterialCatalogue.cpp
```

Перед изменением маршрутов дополнительно искать текущие `CM_Warehouse*Web.h/.cpp` и регистрацию API.

## Калькуляция ремонта

```text
firmware/esp32/src/CM_RepairCosting.h
firmware/esp32/src/CM_RepairCosting.cpp
firmware/esp32/src/CM_RepairCostingWeb.h
firmware/esp32/src/CM_RepairCostingWeb.cpp
```

Ключевые задачи:

- стоимость провода;
- дополнительные материалы;
- исторические снимки цен;
- итоги CU/AL/UNKNOWN;
- контрольные признаки.

## Дополнительные материалы

```text
firmware/esp32/src/CM_MaterialLedger.h
firmware/esp32/src/CM_MaterialLedger.cpp
firmware/esp32/src/CM_MaterialLedgerWeb.h
firmware/esp32/src/CM_MaterialLedgerWeb.cpp
```

Дополнительные реализации могут быть разделены на файлы по функциям, например проверка ремонта или валюты. Перед удалением или переносом искать все определения методов `MaterialLedger`.

## Реестр мастерской

Искать файлы и обработчики, связанные с:

```text
clients
motors
repairs
workshop
```

Данные хранятся под:

```text
/data/workshop/
```

Известный файл:

```text
/data/workshop/repairs.ndjson
```

## Веб-интерфейс

Мобильная версия:

```text
firmware/esp32/web/mobile/
```

Настольная версия:

```text
firmware/esp32/web/desktop/
```

Ключевые страницы:

```text
firmware/esp32/web/mobile/motors.html
firmware/esp32/web/desktop/motors.html
firmware/esp32/web/mobile/writeoff.html
firmware/esp32/web/desktop/writeoff.html
```

Также искать страницы ремонта, выбора двигателя и калькуляции по текущей структуре каталога.

## Хранилища на SD

Журнал намотки:

```text
/data/winding-runs/events.ndjson
```

Дополнительные материалы:

```text
/data/materials/materials.ndjson
/data/materials/materials.tmp
/data/materials/usage.ndjson
/data/materials/usage.pending
/data/materials/adjustments.ndjson
```

Ремонты:

```text
/data/workshop/repairs.ndjson
```

Пути склада провода уточнять по текущим константам в `CM_WarehouseStore`.

## Ключевые тематические документы

Отложенные функции:

```text
docs/46_DEFERRED_UNASSIGNED_WINDINGS_AND_ANALOGUE_MOTORS.md
```

Калькулятор и материалы:

```text
docs/51...
docs/52...
docs/53...
docs/54...
```

Склад и списания:

```text
docs/56...
docs/61...
docs/62...
docs/63...
docs/64...
docs/65...
docs/66_REPAIR_WRITE_OFF_VALUE_TOTALS.md
docs/67_REPAIR_WRITE_OFF_VALUE_TOTALS_UI.md
```

Дополнительные материалы:

```text
docs/68_MATERIAL_USAGE_REPAIR_REFERENCE_INTEGRITY.md
docs/69_MATERIAL_USAGE_COST_PROVENANCE.md
docs/70_MATERIAL_USAGE_COST_FORMULA_METADATA.md
docs/71_MATERIAL_USAGE_VALUE_INTEGRITY.md
docs/72_MATERIAL_USAGE_CURRENCY_PREFLIGHT.md
docs/73_MATERIAL_LEDGER_CORE_INTEGRITY_GUARDS.md
```

Протокол и журнал намотки:

```text
docs/74_WINDING_JOURNAL_TRANSITION_INTEGRITY.md
docs/75_STRICT_UART_EVENT_FIELD_VALIDATION.md
docs/76_BOUNDED_WINDING_JOB_DELIVERY_RETRIES.md
docs/77_UNCONSUMED_JOB_DELIVERY_RESULT_GUARD.md
docs/78_SINGLE_ACTIVE_WINDING_RUN_PER_SESSION.md
```

Следующий планируемый документ:

```text
docs/79_MONOTONIC_WINDING_RUN_IDS_PER_SESSION.md
```

## CI и тесты

В GitHub Actions используются как минимум:

```text
ESP32 Build
CMP Protocol Tests
```

При ошибке открывать конкретный job и читать compile/test log. Общая лента Actions может содержать красные промежуточные коммиты даже после исправления.

## Handoff-каталог

```text
docs/PROJECT_HANDOFF/00_READ_FIRST.md
docs/PROJECT_HANDOFF/01_CURRENT_STATE.md
docs/PROJECT_HANDOFF/02_ARCHITECTURE_AND_HARDWARE.md
docs/PROJECT_HANDOFF/03_PROTOCOL_AND_WINDING_FLOW.md
docs/PROJECT_HANDOFF/04_DATA_STORAGE_API_UI.md
docs/PROJECT_HANDOFF/05_COMPLETED_WORK_LOG.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
docs/PROJECT_HANDOFF/07_BACKLOG_AND_DEFERRED.md
docs/PROJECT_HANDOFF/08_WORK_RULES_AND_VERIFICATION.md
docs/PROJECT_HANDOFF/09_KEY_FILES_INDEX.md
```

После значимой работы обновлять минимум файлы `01`, `05` и `06`.
