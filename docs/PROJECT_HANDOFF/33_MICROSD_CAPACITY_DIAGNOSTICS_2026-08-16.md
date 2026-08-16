# CoilMaster — read-only microSD capacity diagnostics

Дата: **2026-08-16**  
Репозиторий: `FantomeKGZ/CoilMaster`  
Source of truth: `cmp-protocol-v1`

## Контекст

После hardware PASS motor import + persistence проект перешёл к final populated-device acceptance. Для финальной приёмки требовалась явная operator-visible диагностика заполнения microSD без изменения production data lifecycle.

## Реализовано

Добавлен read-only endpoint:

```text
GET /api/system/storage
```

Новые production-файлы:

```text
firmware/esp32/src/CM_StorageDiagnosticsWeb.h
firmware/esp32/src/CM_StorageDiagnosticsWeb.cpp
```

Endpoint возвращает:

- `storage_ready`;
- `card_size_bytes`;
- `filesystem_total_bytes`;
- `filesystem_used_bytes`;
- `filesystem_free_bytes`;
- `automatic_cleanup_allowed=false`.

Модуль не выполняет delete/rename/write/append и не меняет storage policy. Заполнение microSD не запускает автоматическое удаление production данных.

`CM_StaticSiteServer` регистрирует diagnostics helper как отдельный read-only web module.

## UI

Обновлён:

```text
firmware/esp32/web/shared/settings-system-diagnostics.js
```

На desktop и mobile `/settings.html` shared helper уже подключается через `CM_StaticSiteServer`, поэтому обе версии интерфейса получают одинаковые read-only показатели:

- состояние microSD;
- физический размер карты;
- used / total файловой системы;
- свободное место и процент;
- напоминание, что automatic production-data cleanup отключён.

Не вводились arbitrary rotation/deletion thresholds и не создавались автоматические действия при низком свободном месте.

## Regression audit

`Tests/Web/check_web_assets.js` теперь дополнительно проверяет:

- наличие `/api/system/storage`;
- использование `cardSize()`, `totalBytes()`, `usedBytes()`;
- `automatic_cleanup_allowed=false`;
- отсутствие `.remove()`, `.rename()`, `FILE_WRITE`, `FILE_APPEND` в diagnostics module;
- регистрацию модуля в `CM_StaticSiteServer`;
- наличие microSD free-space данных в shared diagnostics UI.

## Коммиты блока

```text
421081826e080cc2b490a8baa952ee0780bb1914  Add read-only microSD diagnostics endpoint
50a0428734b987bac4dab388d316d1de9fc88676  Implement read-only microSD diagnostics endpoint
b2687cc4d5cda8641336ebdc3926fc3b782a0526  Register microSD diagnostics with static site server
cfcf2b7fb2f7f3376a97179f28303b0e9e0e295a  Show read-only microSD capacity diagnostics
0a4e359adecd82d1be166a48f2d53097b51f3c40  Audit read-only microSD diagnostics
```

## Проверка

Firmware-bearing state, включая новый storage module, подтверждён:

```text
ESP32 Build
run: 31938372488
head: cfcf2b7fb2f7f3376a97179f28303b0e9e0e295a
result: SUCCESS
```

Final code/test state подтверждён host CI:

```text
CMP Protocol Tests + web audits
run: 31938393947
head: 0a4e359adecd82d1be166a48f2d53097b51f3c40
result: SUCCESS
```

RAM/Flash numbers из этого run не зафиксированы в handoff и не должны выдумываться.

## Готовность

Текущая оценка CoilMaster v1 остаётся **95%**: motor import persistence hardware gate закрыт, а microSD capacity observability готова к проверке на реальном устройстве. Повышать оценку только из-за host/build проверки этого diagnostics UI не требуется.

## Следующая hardware-проверка

После прошивки firmware-bearing state и обновления `/web`:

1. открыть `Настройки`;
2. убедиться, что diagnostics показывает `microSD: готова`;
3. проверить физический размер, used/total и свободное место;
4. сравнить значения с фактическим объёмом установленной карты на разумность;
5. убедиться, что интерфейс не предлагает и не выполняет automatic cleanup;
6. после этого продолжить final populated-device acceptance / recovery drill.

## Safety-инварианты без изменений

- automatic physical START запрещён;
- ESP32/Web не управляют SSR напрямую;
- auto-resume после reboot запрещён;
- `RUN_COMPLETED` не выполняет automatic wire writeoff;
- wire writeoff остаётся ручным и связан с exact `spool_id + source_session_id + source_run_id`;
- backup restore остаётся operator-only и fail-closed;
- automatic deletion production data при заполнении microSD отсутствует;
- destructive fault injection на рабочей microSD запрещён.
