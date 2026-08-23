# CoilMaster — ESP32 Web API current contract

## 1. Назначение

Web UI обращается к сервисным/data функциям ESP32 только через HTTP API и static assets. Browser code не владеет GPIO, SSR, UART transport или production-файлами microSD напрямую.

Authoritative route source — текущий код `cmp-protocol-v1`, прежде всего:

```text
firmware/esp32/src/main.cpp
firmware/esp32/src/CM_StaticSiteServer.*
firmware/esp32/src/*Web.cpp
```

Этот документ не является автоматически генерируемым exhaustive route list. При расхождении route/method/status authoritative является current source + contract tests.

## 2. Версионирование

Текущие production routes **не** имеют обязательного общего `/api/v1/` prefix. Нельзя добавлять или документировать такой prefix как существующий контракт без реального migration/versioning change.

Несовместимое изменение действующего endpoint требует проверки всех Web consumers, tests и persisted/runtime contracts; rename/remove route без migration proof не является cleanup.

## 3. Route ownership

API разбит по domain owners, а не по одному generic controller:

```text
system/static/recovery        CM_StaticSiteServer.*
network                       CM_NetworkWeb.*
hardware Hall/control         CM_HardwareControlWeb.*
clients/motors/repairs        CM_RepairRegistry*Web.*
winding/job/history           CM_*Winding*Web.*, job/runtime owners
warehouse/spools/writeoff     CM_Warehouse*Web.*
materials                     CM_Material*Web.*
costing/pricing               CM_RepairCostingWeb.* and related owners
backup/restore                CM_Backup*Web.*, CM_RemoteBackupWeb.*
conductor calculator/settings CM_Conductor*Web.*
storage diagnostics           CM_StorageDiagnosticsWeb.*
```

Перед изменением route сначала открывается owning `*Web.cpp`, затем store/service, UI consumers и regression tests.

## 4. Проверенные current examples

Примеры реально зарегистрированных production routes:

```text
GET  /api/system/build
GET  /api/system/network
POST /api/recovery/acknowledge-and-restart

GET  /api/warehouse/write-offs
POST /api/warehouse/write-offs

GET  /api/hardware/hall
POST /api/hardware/hall/refresh
POST /api/hardware/hall/settings
POST /api/hardware/hall/reset
GET  /api/hardware/hall/telemetry
POST /api/hardware/hall/telemetry/start
POST /api/hardware/hall/telemetry/stop
GET  /api/hardware/hall/calibration
POST /api/hardware/hall/calibration/refresh
POST /api/hardware/hall/calibration/arm
POST /api/hardware/hall/calibration/abort
```

Этот список намеренно неполный; он показывает current naming/ownership, а не заменяет source inspection.

## 5. Physical safety boundary

HTTP API никогда не означает physical winding START и никогда не дает ESP32/Web прямое владение SSR.

В частности:

- remote JOB/data operation может подготовить/доставить работу, но physical START остается local/physical Arduino action;
- Hall calibration `arm` переводит calibration в bounded waiting state; движение все равно требует physical START;
- нет Web `SSR test` contract, который позволял бы браузеру напрямую включать SSR;
- timeout/lost ACK не является доказательством Arduino idle;
- reboot/recovery endpoint не может автоматически продолжить winding.

Если новый endpoint затрагивает machine movement, сначала проверяется Arduino ownership/state-machine boundary.

## 6. Writeoff API contract

`POST /api/warehouse/write-offs` — explicit/manual mutation, не следствие `RUN_COMPLETED`.

Для current linked production сервер требует exact persisted provenance:

```text
repair_id
source_session_id
source_run_id
exact immutable spool_id
```

KG_FIRST также требует `spool_id`; historical `UNALLOCATED` records не разрешают новому linked request опустить уже выбранный spool.

Server повторно проверяет OPEN repair, exact completion, immutable selection, duplicate run writeoff и stock identity до confirmed mutation.

## 7. HTTP/error semantics

Каждый domain owner обязан использовать явные HTTP statuses и stable machine-readable `error` identifiers.

Типичные semantics current code:

```text
200 success/read result
202 accepted/queued asynchronous control request
400 malformed/missing/unsupported request fields
404 referenced domain object not found
409 current state conflicts with requested operation
5xx storage/integrity/service failure
503 authoritative store/service unavailable or fail-closed
```

Не следует навязывать один fictitious response envelope вида `{ok,data,error,requestId}` всем endpoints: current APIs используют domain-specific JSON responses.

Operator-controlled strings должны проходить корректное JSON escaping. Internal filesystem details/secrets не должны утекать в ошибках.

## 8. Persistence/mutation rules

HTTP handler не должен считать mutation завершенной до authoritative store result.

Для production data:

- strict input validation до мутации;
- persisted transaction/recovery strategy принадлежит domain store;
- partial/ambiguous state fail-closed;
- confirmed history не удаляется автоматически;
- backup/restore interlock и persisted stale evidence нельзя обходить новым endpoint;
- storage pressure не разрешает автоматическое удаление production data.

## 9. Web consumers

При route change проверяются обе UI variants и shared scripts:

```text
firmware/esp32/web/desktop/
firmware/esp32/web/mobile/
firmware/esp32/web/shared/
```

Desktop/mobile могут различаться presentation, но safety/data semantics одного действия должны совпадать.

## 10. Verification

Изменение API требует минимально:

1. direct owner + call-site review;
2. affected Web consumer review;
3. targeted `Tests/Web/` contract update/addition;
4. ESP32 build when C++ route/runtime code changes;
5. CMP/Arduino gates дополнительно, если API меняет UART/hardware semantics;
6. hardware smoke только когда корректность действительно зависит от физического поведения.

CI GREEN не является hardware GREEN.
