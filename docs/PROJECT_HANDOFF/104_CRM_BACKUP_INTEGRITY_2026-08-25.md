# CoilMaster — CRM backup/export + integrity checkpoint

Дата: **2026-08-25**  
Ветка: **`cmp-protocol-v1`**  
Статус: **GREEN**

## Scope

Закрыт обязательный backup/integrity gate для новых CRM persistence stores перед runtime Material Request mutations.

Release-critical CRM journals теперь входят в backup/export whitelist:

```text
/data/workshop/motor-winding-versions.ndjson
/data/workshop/repair-as-received.ndjson
/data/workshop/material-requests.ndjson
/data/workshop/material-request-movements.ndjson
```

Repair-intake transaction markers не экспортируются как нормальные business data. Их наличие делает snapshot unstable/fail-closed:

```text
/data/workshop/repair-intake.pending.json -> repair_intake_pending
/data/workshop/repair-intake.pending.tmp  -> repair_intake_temp_present
```

## Integrity module

Добавлен read-only `CM_CrmPersistenceIntegrityAudit`.

Он проверяет:

- canonical/complete NDJSON structure;
- monotonic IDs;
- winding-version program validity;
- motor/repair/client cross references;
- AS_RECEIVED repair/client/motor references;
- Material Request repair/client/motor references;
- material-request movement -> exact material_request reference;
- bounded reference batching (`ReferenceBatchSize = 24`);
- fail-closed behavior, без repair/rewrite/delete.

Backup stability gate вызывает:

```text
CrmPersistenceIntegrityAudit::check(storage, crmMetrics)
```

и при ошибке возвращает:

```text
crm_persistence_unstable_or_invalid
```

## Implementation commits

```text
961496b80249448a0da6991639ec2c416d43f97e  add CRM integrity audit header
d78ab42de32272c6bc9d4899856d4bc0ae393779  add CRM integrity audit implementation
ec8431bdf0814329afcc4fb8ec6fdde67ae6517b  include CRM stores in backup/export + recovery markers
0ca776d26c0ed9bb9e523d39c6c479745ab9e011  add permanent backup/integrity regression
8cadd9fdc12219a143a7d776a38a81b78f5fc72f  wire regression into permanent CMP workflow
2ac02ad6f1b84f1f3309574944400203e0482cb4  current firmware verification head
```

## Permanent regression

```text
Tests/Web/check_crm_backup_integrity.js
```

CMP workflow step:

```text
Audit CRM backup and integrity coverage
```

Regression protects export entries, repair-intake recovery markers, fail-closed audit wiring, bounded reference batching and read-only behavior.

## Verified CI

Firmware verification head:

```text
2ac02ad6f1b84f1f3309574944400203e0482cb4
```

Confirmed:

```text
CMP Protocol Tests run 32855540935 / SUCCESS
ESP32 Build run 32855541246 / SUCCESS
```

Earlier workflow-only permanent-regression wiring also passed:

```text
CMP run 32855462871 / SUCCESS
```

## One-shot workflow cleanup note

The temporary guarded patch workflow successfully applied the large `CM_BackupExportWeb.cpp` change using exact blob SHA. GitHub connector deletion of that workflow is blocked by tool safety controls. It was therefore neutralized in commit:

```text
35572256ff815e5ee8cc74cd4c968daa3d23528d
```

Current file is manual `workflow_dispatch` only, `contents: read`, with no patch/push/write behavior. It cannot mutate the branch automatically.

## Safety impact

No machine-control safety semantics changed.

Still invariant:

- no automatic physical START;
- no automatic START between repeats;
- no auto-resume after reboot;
- Arduino owns SSR;
- ESP32/Web never directly controls SSR;
- `RUN_COMPLETED` never automatically deducts material;
- warehouse ISSUE requires explicit operator action;
- run-linked wire movement preserves exact source session/run provenance;
- restore remains operator-only, transactional and fail-closed.

## Next

The next active block is the generic warehouse item catalog / unit-accounting contract. Existing `MaterialLedger` is the preferred foundation rather than introducing a duplicate catalog, but its current material serialization must first be corrected and regression-protected before it is reused by Material Request runtime APIs.
