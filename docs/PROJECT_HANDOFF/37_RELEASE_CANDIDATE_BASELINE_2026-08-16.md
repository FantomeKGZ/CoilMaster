# CoilMaster — release-candidate deployment baseline

Дата: **2026-08-16**  
Репозиторий: `FantomeKGZ/CoilMaster`  
Source of truth: `cmp-protocol-v1`

## Назначение

Этот checkpoint фиксирует точку, которая реально прошла final populated-device hardware acceptance, и отделяет production firmware/web от последующих test/docs-only commits.

## Реально проверенный ESP32 + Web production baseline

Последний commit, который менял production ESP32/Web перед финальной аппаратной приёмкой:

```text
cfcf2b7fb2f7f3376a97179f28303b0e9e0e295a
Show read-only microSD capacity diagnostics
```

Этот state включает:

- read-only `/api/system/storage`;
- microSD capacity/used/free diagnostics;
- shared desktop/mobile settings diagnostics UI;
- все production изменения, накопленные до этого commit в `cmp-protocol-v1`.

Подтверждённый firmware build:

```text
ESP32 Build
run: 31938372488
head: cfcf2b7fb2f7f3376a97179f28303b0e9e0e295a
result: SUCCESS
```

Пользователь после этого прошил актуальный firmware-bearing state, обновил `/web`, подтвердил microSD diagnostics и затем успешно прошёл final populated-device acceptance / recovery drill.

Следовательно `cfcf2b7...` является зафиксированным ESP32/Web production deployment baseline для данного final hardware PASS.

## Что изменилось после production baseline

Сравнение `cfcf2b7...` с checkpoint-36 HEAD `ef1ff229a9341fcf2da4ee23160da6e394b82f04` показывает только:

- `.github/workflows/cmp-protocol-tests.yml`;
- `Tests/Web/check_web_assets.js`;
- `Tests/Web/check_final_acceptance_contracts.js`;
- `docs/PROJECT_HANDOFF/...` checkpoints / entrypoint.

После `cfcf2b7...` production firmware/web файлы не менялись.

Поэтому commits после baseline не требуют повторной прошивки или повторения final hardware acceptance, пока production paths остаются неизменными.

## Repo / CI baseline после аппаратной приёмки

Final repo-level acceptance protection:

```text
Tests/Web/check_web_assets.js
Tests/Web/check_release_contracts.js
Tests/Web/check_final_acceptance_contracts.js
```

Подтверждённый CI перед hardware PASS:

```text
CMP Protocol Tests
run: 31940069683
head: f2487099c574be5aad3b17dd38330c5f67591e2f
result: SUCCESS
```

В run прошли protocol tests, web audit, release safety contracts и final acceptance contracts.

Checkpoint hardware PASS:

```text
36_FINAL_POPULATED_DEVICE_ACCEPTANCE_PASS_2026-08-16.md
commit: ef1ff229a9341fcf2da4ee23160da6e394b82f04
```

## Arduino production baseline

Production entry point остаётся:

```text
firmware/arduino/src/main.cpp
```

Arduino hardware path, physical START, SSR authority и UART winding flow были реально подтверждены ранее, а final release contract audit продолжает защищать эти invariants.

Точный отдельный commit SHA прошивки Arduino, физически находящейся на устройстве во время final populated-device drill, в текущих checkpoints не был записан. Его **не выдумывать**.

До изменения Arduino production code повторная прошивка только ради test/docs commits не требуется.

## Safety / recovery baseline

Release candidate сохраняет обязательные свойства:

- physical START только физический;
- SSR authority только Arduino state machine;
- ESP32/Web не управляют SSR напрямую;
- boot не выполняет auto-resume;
- `RUN_COMPLETED` не выполняет automatic wire writeoff;
- writeoff manual + exact `spool_id + source_session_id + source_run_id`;
- backup restore operator-only, transactional, fail-closed;
- reboot не продолжает restore/apply автоматически;
- persisted restore evidence блокирует новые backup/restore операции до explicit cleanup;
- microSD filling не запускает automatic deletion production data.

## Release-candidate status

Final populated-device hardware acceptance закрыт.

Текущая readiness assessment остаётся **98%** до окончательного release closure. Не повышать до 100% только из-за создания документа.

Оставшиеся небольшие release-closure пункты:

1. при необходимости зафиксировать точный Arduino flashed revision при следующей плановой прошивке, не перепрошивая только ради номера SHA;
2. отдельно подтвердить `http://coil.local/`, если mDNS должен считаться обязательным release convenience; IP fallback остаётся рабочим требованием;
3. destructive corruption/power-loss/fault-injection — только на disposable microSD/image и как отдельный hardening, не на рабочей карте;
4. после закрытия выбранных release-closure пунктов оформить final v1 release status без изменения safety invariants.
