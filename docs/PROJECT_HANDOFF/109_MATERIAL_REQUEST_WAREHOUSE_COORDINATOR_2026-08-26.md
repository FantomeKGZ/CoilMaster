# CoilMaster — Material Request warehouse coordinator

Дата: **2026-08-26**  
Ветка: **`cmp-protocol-v1`**  
Статус: **SOFTWARE GREEN**

## Реализовано

Добавлен crash-safe `MaterialRequestWarehouseCoordinator`, который связывает immutable Material Request movement evidence с физическим изменением `MaterialLedger`.

Transaction ordering:

```text
explicit operator action
-> durable material-request-warehouse.pending.json
-> append immutable Material Request movement
-> mutate MaterialLedger
-> verify movement evidence + ledger evidence
-> clear pending marker
```

Recovery matrix:

```text
movement=false, ledger=false -> safe clear/retry
movement=true,  ledger=false -> complete ledger mutation
movement=false, ledger=true  -> impossible ordering / fail-closed
movement=true,  ledger=true  -> clear pending
```

Coordinator does not trust Web-supplied prices. It resolves the ACTIVE MaterialLedger item, performs canonical unit conversion and records an exact cost snapshot in the immutable movement.

Warehouse mutation is allowed only while the Material Request is `DRAFT` or `ISSUED`; `PRICED` and `CLOSED` freeze physical changes. Repair must still be OPEN.

Movement contract was tightened:

- every movement has `transaction_ref`;
- `CORRECTION` persists explicit `ADD` or `REMOVE`;
- `RUN_WIRE` is `ISSUE`/`KG` only;
- `RUN_WIRE` preserves exact `source_session_id + source_run_id`, CU/AL and wire diameter;
- `RUN_COMPLETED` remains non-mutating.

Stable backup blocks while either warehouse pending marker exists.

## Main files

```text
firmware/esp32/src/CM_MaterialRequestWarehouseCoordinator.h
firmware/esp32/src/CM_MaterialRequestWarehouseCoordinator.cpp
firmware/esp32/src/CM_MaterialRequestWarehousePendingStore.h
firmware/esp32/src/CM_MaterialRequestWarehousePendingStore.cpp
firmware/esp32/src/CM_MaterialRequestMovementStore.h
firmware/esp32/src/CM_MaterialRequestMovementStore.cpp
firmware/esp32/src/CM_CrmPersistenceIntegrityAudit.cpp
Tests/Web/check_material_request_warehouse_coordinator.js
```

## Verification

Permanent coordinator regression is wired into `CMP Protocol Tests`.

Evidence:

```text
CMP run 32925574132 / SUCCESS
CMP run 32925599254 / SUCCESS
```

Earlier pending-store and firmware evidence remains:

```text
ESP32 Build run 32861148982 / SUCCESS
CMP run 32861266055 / SUCCESS
backup guard patch run 32861669436 / SUCCESS
```

The failed one-shot run `32863289280` was not a production regression: GitHub rejected an Actions-bot attempt to modify `.github/workflows/cmp-protocol-tests.yml` because that token lacked workflow permission. The permanent regression was then wired directly through the GitHub connector and passed.

## Next

Expose bounded runtime/Web API for:

```text
create Material Request
read request / repair requests
read movement history
read/transition lifecycle
explicit ISSUE / RETURN / CORRECTION through coordinator only
```

No Web endpoint may directly mutate `MaterialLedger` for a Material Request or append request movements outside the coordinator.

Current exact-spool production writeoff remains authoritative until the later coordinated migration block.
