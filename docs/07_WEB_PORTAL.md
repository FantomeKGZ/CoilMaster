# CoilMaster — Web portal current structure

## 1. Web root и выбор интерфейса

ESP32 обслуживает static assets из `/web`. Корневой `firmware/esp32/web/index.html` предлагает/запоминает вариант интерфейса:

```text
/desktop/
/mobile/
```

Production web tree:

```text
firmware/esp32/web/
├── index.html
├── desktop/
├── mobile/
├── shared/
├── reference/
│   └── motor-reference.json
└── sites/
    └── reference/
        ├── desktop/
        └── mobile/
```

`reference/motor-reference.json` — generated read-only data index. `sites/reference/` — отдельный runtime-owned reference UI; `CM_StaticSiteServer` маршрутизирует `/sites/reference[/]` к desktop/mobile variant. Эти каталоги не являются cleanup-дубликатами друг друга.

## 2. Static/runtime ownership

Основные owners:

```text
firmware/esp32/src/CM_StaticSiteServer.*
firmware/esp32/web/desktop/
firmware/esp32/web/mobile/
firmware/esp32/web/shared/
Tests/Web/
```

Static page не имеет права напрямую обращаться к GPIO, SSR, UART object или production-файлам microSD. Все dynamic operations идут через controlled ESP32 HTTP API.

## 3. Desktop/mobile parity

Desktop и mobile — два presentation variants одного production flow. Они могут отличаться layout/navigation, но должны совпадать по data/safety semantics.

При изменении operator-facing capability проверяются:

```text
firmware/esp32/web/desktop/<page>
firmware/esp32/web/mobile/<page>
firmware/esp32/web/shared/<feature>.js   если применимо
Tests/Web/                               соответствующий contract audit
```

Нельзя исправлять только одну variant, если действие доступно в обеих.

## 4. Shared JavaScript

`firmware/esp32/web/shared/` содержит реальные feature owners, а не автоматически удаляемые helpers. Current tree включает app-shell, backup, Hall/settings, history, costing и writeoff helpers.

Удаление shared file допустимо только после проверки:

- static HTML references;
- dynamic injection из `CM_StaticSiteServer`;
- `app-shell.js`/других shared loaders;
- desktop/mobile consumers;
- Web contract tests.

Именно по этому правилу старый calculator helper был удален ранее, а текущие shared modules остаются `KEEP`.

## 5. Winding/job UI boundary

Web может подготовить linked winding job и доставить data на ESP32/Arduino pipeline. Current production flow до UART:

```text
client -> motor -> OPEN repair -> linked winding
-> immutable job/session identity
-> immutable snapshot
-> exact immutable spool selection
-> UART JOB
```

Web/UI не может считать job creation/delivery физическим запуском.

Physical winding START остается local/physical Arduino action. ESP32/Web никогда напрямую не управляют SSR.

## 6. Run/history presentation

Authoritative фактические события сохраняются run-level:

```text
RUN_STARTED(session_id, run_id, ...)
RUN_COMPLETED(session_id, run_id, ...)
```

UI может группировать/агрегировать одинаковые программы для удобства, но исходные run evidence не объединяются физически и не удаляются.

Lost ACK/timeout не дает UI права показывать Arduino как гарантированно idle или инициировать автоматический новый run.

## 7. Material/writeoff UI

После `RUN_COMPLETED` Web может предложить operator manual writeoff, но stock не меняется автоматически.

Для current linked production writeoff обязан сохранить:

```text
source_session_id + source_run_id + exact immutable spool_id
```

Desktop/mobile writeoff UI не могут превращать historical `UNALLOCATED` evidence в новый optional-spool production path.

## 8. Motors / repairs / costing

Web portal предоставляет текущие страницы для clients, motors, repairs, winding jobs/history, warehouse/materials, costing/pricing, reports и settings. Exact field set/validation определяется текущими API/store owners и matching Web contract tests, а не старой aspirational формой в документации.

Motor reference (`winding-reference.html`) читает только `/reference/motor-reference.json`; этот reference index имеет `reference_only` semantics, не создает рабочий motor record и не содержит `coil_program` для станка.

## 9. Settings / hardware service UI

Current hardware-control Web owner — `CM_HardwareControlWeb.*`. Он предоставляет bounded Hall settings/telemetry/calibration operations через Arduino CMP1 boundary.

Web может:

- читать Hall settings/state;
- запросить refresh;
- изменить разрешенные Hall settings через validated control path;
- читать/start/stop telemetry;
- arm/refresh/abort Hall calibration.

Web **не имеет direct SSR test/control API**. Calibration arm не запускает двигатель: состояние ожидает physical START на Arduino.

Не документировать browser-controlled SSR как service feature.

## 10. Backup/recovery/settings

Web предоставляет operator-facing backup/restore/network/settings workflows через соответствующие ESP32 owners. Для них сохраняются общие правила:

- restore explicit/operator-only;
- apply transactional/fail-closed;
- persisted stale evidence блокирует небезопасное продолжение;
- reboot не auto-continues restore/apply;
- low storage не запускает автоматическое удаление production data.

## 11. UX safety rules

- destructive/irreversible действия требуют явного operator intent;
- UI не показывает physical result до подтвержденного hardware/persisted evidence;
- write mutation показывает success/error из authoritative API result;
- color не является единственным обозначением критического состояния;
- monitoring не должен превращаться в movement authority;
- reconnect/reload не запускает winding заново;
- desktop/mobile должны сохранять одинаковую safety semantics.

## 12. API

Current API routing документирован в `docs/08_API.md` и определяется текущими `*Web.cpp` owners. Нет обязательного общего `/api/v1/` prefix и нет generic Web access к GPIO/UART/filesystem.

## 13. Verification

Web change требует соответствующего `Tests/Web/` audit. C++ static/API changes требуют ESP32 Build; CMP/hardware semantics additionally требуют protocol/Arduino gates. Hardware GREEN никогда не выводится только из Web/CI tests.
