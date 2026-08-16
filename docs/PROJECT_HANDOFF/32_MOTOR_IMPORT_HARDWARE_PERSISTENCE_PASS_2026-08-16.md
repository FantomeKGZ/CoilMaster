# CoilMaster — motor import hardware persistence PASS

Дата: **2026-08-16**  
Репозиторий: `FantomeKGZ/CoilMaster`  
Source of truth: `cmp-protocol-v1`

## Hardware gate закрыт

Пользователь подтвердил motor import на реальном ESP32:

1. тестовая JSON-запись была успешно импортирована через штатный motor-import UI;
2. импортированная запись появилась в базе двигателей;
3. ESP32 был перезагружен;
4. после reboot импортированная запись осталась доступна в базе.

Результат: **MOTOR IMPORT + PERSISTENCE AFTER REBOOT = HARDWARE PASS**.

Повторять этот gate не требуется, если код motor-import/persistence не меняется.

## Что подтверждает этот тест

Аппаратно подтверждены вместе:

- JSON import path на реальном ESP32;
- запись через production `POST /api/motors` flow;
- persistence записи на microSD;
- корректное чтение записи после reboot;
- отсутствие зависимости импортированной записи только от volatile RAM state.

Host-side importer validation/audit уже был защищён release-contract CI в checkpoint `31`; этот checkpoint закрывает оставшуюся real-device часть.

## Что этот тест не подтверждает автоматически

Не считать из этого автоматически проверенными:

- destructive corruption/fault injection;
- работу на физически неисправной microSD;
- все возможные большие import packages до лимита 50 записей;
- любые будущие изменения motor-import/persistence кода.

Destructive fault injection по-прежнему разрешён только на disposable card/image.

## Готовность

После закрытия positive transactional restore apply и motor-import persistence hardware gates текущая оценка CoilMaster v1: **95%**.

Основные production flow, backup/restore safety boundary, exact spool/writeoff provenance, hardware winding path, positive transactional restore и motor import persistence теперь подтверждены.

## Следующий release gate

Следующий обязательный этап — **final populated-device acceptance / recovery drill** без destructive fault injection на рабочей microSD.

Цель финальной приёмки — подтвердить уже собранные подсистемы как единый эксплуатационный набор на устройстве с тестовыми production-данными, не повторяя без причины уже закрытые hardware gates.

Минимальный безопасный acceptance scope:

1. после обычного reboot открыть основные разделы и подтвердить доступность clients / motors / repairs / warehouse / winding history;
2. проверить, что нет automatic physical START и SSR не активируется от Web/ESP32;
3. проверить существующую OPEN/CLOSED repair историю и linked winding данные;
4. проверить manual writeoff provenance для сохранённых run records;
5. выполнить обычный fresh backup и убедиться, что batch читается/инспектируется;
6. подтвердить, что после reboot никакой restore/apply не продолжается автоматически;
7. проверить diagnostics/network/time/UI на отсутствие release-blocking ошибок.

Уже подтверждённые hardware gates не нужно заново выполнять как отдельные тесты, если соответствующий production-код после них не менялся.

## Safety-инварианты без изменений

- automatic physical START запрещён;
- ESP32/Web не управляют SSR напрямую;
- auto-resume после reboot запрещён;
- `RUN_COMPLETED` не выполняет automatic wire writeoff;
- wire writeoff остаётся ручным и связан с exact `spool_id + source_session_id + source_run_id`;
- backup restore остаётся operator-only и fail-closed;
- destructive recovery/fault injection на рабочей microSD запрещён.
