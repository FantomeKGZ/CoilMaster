# CoilMaster — Hall calibration history on ESP32 SD

Дата: **2026-08-24**  
Ветка: **`cmp-protocol-v1`**

## Назначение

История расширенного Hall test/calibration хранится только на ESP32 microSD. Arduino Uno не получает новую history/persistence нагрузку и продолжает хранить только текущий authoritative Hall profile в EEPROM.

## Storage contract

Файл:

```text
/data/hardware/hall-calibration-history.ndjson
```

Bounded retention:

```text
MaxEntries = 10
```

При добавлении 11-й записи удаляется самая старая. Запись привязана к exact `measurement_id`.

Atomic replacement использует:

```text
hall-calibration-history.tmp
hall-calibration-history.bak
```

Recovery придерживается fail-closed transaction semantics: валидный main имеет приоритет; valid backup восстанавливает предыдущий authoritative state; prepared temp не вытесняет валидный backup.

## Поля результата

Каждая запись сохраняет:

- `measurement_id`;
- monotonic `recorded_at_ms`;
- baseline/min/max;
- sample count;
- duration;
- ESP32 recommendation validity;
- recommended threshold/hysteresis/direction;
- final apply result;
- отдельно authoritative persisted Arduino profile после успешного `CAL_APPLIED`.

Persisted profile содержит:

- threshold;
- hysteresis;
- release debounce;
- direction.

Recommendation и persisted profile намеренно разделены. Полученный/рассчитанный test result не означает, что EEPROM был изменён.

## Runtime lifecycle

```text
CAL_RESULT
 -> ESP32 analyzer
 -> history measurement record

CAL_PROPOSAL
 -> WAITING_APPLY_CONFIRM
 -> local # on Arduino
 -> CAL_APPLIED
 -> exact history finalize
```

Abort, cancellation, timeout и persistence failure сохраняются как итог результата без ложного persisted profile.

## Web API

```text
GET /api/hardware/hall/calibration/history
```

Возвращает максимум 10 записей, newest-first.

## Hall Web UI

Общий controller:

```text
firmware/esp32/web/shared/settings-hall-calibration.js
```

Desktop и mobile Hall-страницы получают один и тот же блок `Последние калибровки` без дублирования runtime logic. UI показывает отдельно:

- измерение `baseline/min/max/span/samples/duration`;
- recommendation ESP32;
- final apply result;
- authoritative Arduino EEPROM profile, только когда `persisted_valid=true`.

История перечитывается при открытии Hall page и после terminal apply/abort/timeout.

## SD winding reference navigation

Большой справочник на microSD открывается напрямую:

```text
/sites/reference/desktop/
/sites/reference/mobile/
```

Shared app shell уже использует эти маршруты. Дополнительно исправлены legacy navigation owners:

- desktop home sidebar получил `📚 Справочник`;
- mobile `Ещё` переведён со старого `/mobile/winding-reference.html` на `/sites/reference/mobile/`.

## Safety boundary

История и UI не изменяют:

- physical START ownership;
- SSR ownership;
- Hall realtime counting ownership;
- EEPROM apply confirmation;
- no-auto-resume policy;
- no-auto-start policy.

Arduino Uno Flash/RAM этим блоком не расширяются: implementation находится только в `firmware/esp32/src` и Web assets.

## Current implementation commits

```text
95ee484f  add bounded calibration history store header
ded1a0cd  persist last ten calibration results
63ec23de  expose history API owner
5e77188a  initialize history owner
4db40430  track abort finalization
ecccebdc  persist/expose live calibration history
f8f5682d  route mobile menu to SD winding reference
1801b4a4  expose SD winding reference in desktop menu
7ebc3704  show bounded Hall calibration history in shared UI
bd556000  guard Hall history and SD reference links
5897dbf3  run Hall history/reference regression in CI
```

## Verification status

Пользователь 2026-08-24 подтвердил GREEN для предыдущего runtime/history checkpoint до текущего UI follow-up. Текущий UI/regression follow-up (`1801b4a4`..`5897dbf3`) требует своих Actions; не переносить GREEN автоматически на новый HEAD до evidence.

Последний подтверждённый Uno baseline остаётся:

```text
RAM   1219 / 2048
Flash 32170 / 32256
```

Промежуточный hardware test не требуется. Полный hardware acceptance остаётся финальным gate после завершения software migration/optimization.
