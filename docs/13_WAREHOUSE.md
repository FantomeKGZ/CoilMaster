# CoilMaster — склад провода и списание на фактический запуск

## 1. Текущая модель учета

Провод учитывается по массе. Для физических катушек/бобин authoritative складская сущность — **spool** с неизменяемым `spool_id`; масса хранится в граммах, UI может показывать граммы или килограммы.

Текущий linked-production flow не допускает анонимное списание провода после намотки. До UART boundary для сеанса сохраняется immutable material selection:

```text
job_id + session_id + repair_id + motor_id
+ exact spool_id + diameter + wire_type + weight_at_selection
```

Production owner:

```text
firmware/esp32/src/CM_JobSpoolSelectionStore.*
firmware/esp32/src/CM_WarehouseStore.*
firmware/esp32/src/CM_WarehouseWriteOff*.cpp
```

## 2. Spool identity

Для активной катушки сохраняются как минимум:

- уникальный `spool_id`;
- материал провода (`CU` / `AL`);
- диаметр;
- текущая масса;
- складская/ценовая информация, относящаяся к текущей модели склада.

Исторические записи и движения не переписываются только потому, что текущие свойства склада позднее изменились.

`CM_WarehouseLegacySpoolMaterial.cpp` — действующий migration owner для старых ACTIVE spool без `wire_type`. Наличие слова `Legacy` в имени не делает этот модуль dead code.

## 3. Приход и изменение остатков

Складские мутации выполняются через authoritative `WarehouseStore` и сохраняют movement/history evidence. Подтвержденные движения не должны физически исчезать; корректировки выполняются отдельными операциями с сохранением причины и истории.

Нельзя обходить store прямой записью файлов microSD из Web/UI.

## 4. Linked-production manual writeoff

После фактического `RUN_COMPLETED` материал **не списывается автоматически**.

Оператор выполняет отдельное подтвержденное списание. Для текущего linked production обязательна exact provenance:

```text
source_session_id + source_run_id + exact immutable spool_id
```

Перед мутацией `WarehouseStore` fail-closed проверяет:

1. warehouse/store готов;
2. `repair_id` существует и ремонт остается `OPEN`;
3. immutable spool selection для `source_session_id` существует;
4. selection принадлежит тому же repair и содержит тот же `spool_id`;
5. exact `source_run_id` действительно завершен;
6. для этой пары session/run еще нет подтвержденного writeoff;
7. цена склада сконфигурирована;
8. requested spool/diameter/material согласованы с authoritative spool identity;
9. расход не уничтожает/не превышает текущий доступный остаток.

Любая неуспешная проверка отвергает операцию до нормального confirmed результата.

## 5. Два поддерживаемых способа ввода фактической массы

### До/после взвешивания spool

Оператор указывает фактический вес до и после. Расход:

```text
consumed_g = weight_before_g - weight_after_g
```

`weight_after_g` обязан быть меньше `weight_before_g`; writeoff привязан к exact spool + session + run.

### KG_FIRST

Оператор указывает фактически использованную массу (`quantity_kg` / нормализованную массу в граммах). Для текущего linked production KG_FIRST также обязан сохранить exact immutable `spool_id` из pre-UART selection.

Исторические `UNALLOCATED` KG_FIRST записи остаются read/audit/recovery compatibility evidence. Они **не** разрешают новому linked run опустить spool после `RUN_COMPLETED`.

Если когда-либо понадобится настоящий новый unallocated production workflow, его immutable material provenance должна быть определена **до UART boundary**, а не создана post-run fallback-логикой.

## 6. Транзакционность writeoff

Списание spool выполняется как bounded append/mutation transaction:

```text
PENDING movement
-> durable spool weight mutation
-> CONFIRMED movement
```

Если финальный append не удался и массу удалось вернуть, transaction закрывается `ABORTED`. Если состояние становится неоднозначным, store переходит в fail-closed/not-ready состояние до reconciliation/recovery.

Это защищает от частичного списания при сбое питания или записи microSD.

## 7. Цена и историческая стоимость

Стоимость расхода рассчитывается из сохраненной warehouse price snapshot для операции, а не задним числом из будущей текущей цены.

Базовая формула для массы в граммах:

```text
cost = consumed_g * price_per_kg / 1000
```

Денежные значения хранятся в целых minor units с контролируемым округлением там, где это предусмотрено доменным модулем.

## 8. Связь с ремонтом

Текущая production provenance:

```text
Client
  -> Motor
    -> OPEN Repair
      -> linked Winding Session
        -> immutable exact Spool Selection
          -> physical RUN_STARTED / RUN_COMPLETED
            -> explicit Manual Writeoff
               (source_session_id + source_run_id + spool_id)
```

За счет exact run-level provenance один завершенный run не может быть ошибочно засчитан как списание другого run того же сеанса.

## 9. Контроль ошибок и integrity

Обязательные правила:

- нет автоматического списания из `RUN_COMPLETED`;
- нет нового linked writeoff без exact session/run/spool provenance;
- duplicate writeoff для exact source run отвергается;
- нельзя списать отсутствующий/несовпадающий spool;
- нельзя списать нулевую/отрицательную массу;
- нельзя принять расход, который не оставляет допустимый текущий остаток spool;
- repair должен быть `OPEN` во время writeoff;
- confirmed/history evidence не удаляется автоматически;
- warehouse persistence и movement logs входят в integrity/backup coverage;
- low-space condition не дает права автоматически удалять production data.

## 10. UI/API

Desktop/mobile warehouse/writeoff UI являются клиентами authoritative ESP32 API и не владеют складским состоянием самостоятельно.

Relevant owners/tests:

```text
firmware/esp32/src/CM_WarehouseWeb.*
firmware/esp32/src/CM_WarehouseSpoolWeb.cpp
firmware/esp32/src/CM_WarehouseWriteOffWeb.cpp
firmware/esp32/web/desktop/warehouse.html
firmware/esp32/web/mobile/warehouse.html
firmware/esp32/web/desktop/writeoff.html
firmware/esp32/web/mobile/writeoff.html
Tests/Web/check_kg_first_material_contracts.js
Tests/Web/check_writeoff_fault_contracts.js
```

Любое изменение writeoff semantics должно сохранять manual exact-run exact-spool boundary и иметь targeted regression verification.
