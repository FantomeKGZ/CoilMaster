# Где остановились и что делать дальше

Дата обновления: **2026-08-10**  
Ветка: **`cmp-protocol-v1`**

Код ветки — единственный source of truth. `main` не использовать как источник реализации. Перед каждым изменением существующего файла заново fetch актуальный blob из `cmp-protocol-v1` и использовать текущий SHA.

## Текущий подтверждённый hardware checkpoint

Arduino Uno после SRAM-оптимизации работает стабильно:

```text
CM_BOOT stage=READY free_sram=357
CM_ALIVE ... free_sram=366
```

Reset-loop устранён. Пользователь дополнительно подтвердил реальную работу:

```text
локальное создание программы на Arduino
→ физический START
→ выполнение намотки
→ LOCAL_EVT
→ отправка на ESP32
```

Постоянный звук buzzer оказался не firmware bug: пищалка была физически подключена к **D12**, который является SSR output. После переноса на **D11** проблема устранена.

Актуальная Arduino pin-map:

```text
D11 = Buzzer
D12 = SSR
A0  = Hall
A1  = Arduino TX → ESP32 RX
A2  = Arduino RX ← ESP32 TX
```

Safety boundary не менялся:

- physical START только физический;
- ESP32/Web не управляют SSR напрямую;
- auto-resume после reboot отсутствует;
- `RUN_COMPLETED` не выполняет automatic wire writeoff;
- wire writeoff остаётся ручным и exact-run/exact-spool.

## Автономные намотки Arduino

Production path для standalone Arduino задания уже реализован:

```text
Arduino local keypad
→ physical START
→ RUN_STARTED / RUN_COMPLETED
→ LOCAL_EVT с winding_type + coil_count + program
→ ESP32 autonomous archive
→ поиск программы ±20%
→ existing/similar motor
→ ручная assignment completed task → motor
```

Persistent files:

```text
/data/autonomous-windings/events.ndjson
/data/autonomous-windings/assignments.ndjson
```

Incomplete `RUN_STARTED` records видны, но не могут быть назначены как completed evidence. Replayed `RUN_COMPLETED` после пропущенного START сохраняется с `start_observed=0` и предупреждением.

Последний protocol hardening:

```text
a9fb6e395834c048d251f0475c27103b7c0160ad
```

ESP32 UART parser теперь fail-closed требует:

```text
RUN_STARTED   -> completed_runs == 0
RUN_COMPLETED -> completed_runs > 0
```

## Backup / deep integrity — текущий блок

Добавлен read-only authoritative audit автономного архива:

```text
44a3b65114a1e29edcb7e78508c15986e747426f
0748b74563dfc4ff90f9924e159017b37b8e73b2
```

API внутри firmware:

```text
AutonomousWindingArchive::validateStorage(...)
```

Audit не вызывает `begin()`, не создаёт каталоги и ничего не изменяет на microSD. Он использует production parsers/validators для `events.ndjson` и `assignments.ndjson`.

Backup integration:

```text
55318f1a6366e5163739e4b0447da13d95f76870
```

В read-only whitelist теперь входят:

```text
autonomous-winding-events
→ /data/autonomous-windings/events.ndjson

autonomous-winding-assignments
→ /data/autonomous-windings/assignments.ndjson
```

`/api/backup/manifest` дополнительно публикует:

```text
autonomous_winding_archive_audit_duration_ms
autonomous_winding_event_record_count
autonomous_winding_started_record_count
autonomous_winding_completed_record_count
autonomous_winding_assignment_record_count
```

Если archive integrity не доказана:

```text
snapshot_stability_reason = autonomous_winding_archive_unstable_or_invalid
```

Mobile/desktop backup UI умеет объяснять эту причину:

```text
f4225a6d161675ed3314fa9925953461cad02d80
b5848051146e778ef5fb072bfae22a5f5bc710e5
```

## UI version switching

Переключение mobile ↔ desktop теперь добавляется централизованно `CM_StaticSiteServer` в конец каждой `/mobile/...` и `/desktop/...` HTML-страницы. Сохраняются текущий path/query/hash.

Текущий подтверждённый пользователем блок работает.

Ключевой commit:

```text
51f3e6d122d5bb7e48145f9cc5243f6ad4122050
```

Для этого переключателя HTML на microSD менять не требуется — footer инжектируется firmware server-side.

## Production flow

```text
client → motor → OPEN repair → costing → linked winding → exact spool_id
→ immutable snapshot + spool-selection → UART → physical START
→ RUN_STARTED/RUN_COMPLETED → manual exact-run wire writeoff
→ materials/pricing → finalization preflight
→ CLOSED → archive/report → read-only backup
```

Autonomous Arduino archive — отдельный ancillary flow и не ослабляет строгий linked-repair path.

## Что сейчас НЕ подтверждено

Текущий ESP32 HEAD после новых backup/protocol changes ещё не имеет подтверждённого local build или GREEN CI result.

```text
BUILD NOT CONFIRMED
CI NOT CONFIRMED
```

GitHub combined status через connector ранее возвращал пустой список.

## Следующее действие

Сначала clean ESP32 build текущего HEAD:

```powershell
pio run -e esp32 -t clean
pio run -e esp32
```

Если build успешен:

```powershell
pio run -e esp32 -t upload
```

После upload проверить:

1. `/api/backup/manifest` содержит два autonomous archive items;
2. `snapshot_stable=true` на целой карте;
3. новые autonomous record counts не `null` при выполненном audit;
4. скачиваются `autonomous-winding-events.ndjson` и `autonomous-winding-assignments.ndjson`;
5. обычный local Arduino task по-прежнему попадает в archive;
6. mobile ↔ desktop footer остаётся рабочим.

После этого продолжить полный production E2E:

```text
linked repair
→ exact spool
→ JOB_ACK
→ physical START
→ RUN_STARTED
→ RUN_COMPLETED
→ manual exact-run wire writeoff
→ costing
→ finalization preflight
→ CLOSED
→ stable backup
```

На реальном dataset сохранить один manifest с audit durations/counts/bytes. Performance/rotation решения принимать только после измерений. Database migration и arbitrary rotation threshold преждевременно не вводить.
