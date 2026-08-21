# Backlog и отложенные функции

Дата актуализации: **2026-08-21**  
Ветка: `cmp-protocol-v1`

Этот файл содержит только **реально отложенные/неутверждённые** направления. Он не является текущей очередью работ.

Текущая работа выбирается из:

```text
docs/PROJECT_HANDOFF/00_READ_FIRST.md
docs/PROJECT_HANDOFF/61_CURRENT_RECOVERY_AND_DOCS_BASELINE_2026-08-21.md
docs/PROJECT_HANDOFF/06_ACTIVE_WORK_AND_NEXT_STEPS.md
explicit current user request
concrete current failure
```

Старые пункты этого файла про job linkage, immutable snapshot, persisted recovery, autonomous archive, pagination, Wi-Fi/AP, FTP, backup, KG_FIRST, writeoff hardening и другие уже реализованные блоки удалены из backlog, чтобы новый AI не начинал их заново.

## Не считать backlog

Следующие области уже реализованы/закрыты на repo level и не должны автоматически превращаться в новую задачу:

- persistent linked job identity/state/snapshot;
- repeat_target and one-physical-START-per-run semantics;
- JOB cancel/recovery with `ALREADY_CLEAR` / safe `ALL_CLEAR`;
- Arduino autonomous winding archive and assignment UI;
- motor schema/import/detail/repair-history flows;
- bounded/paged growing collection APIs;
- network profile manager/fallback AP;
- outgoing remote backup FTP and incoming `/web` recovery FTP foundation;
- read-only backup/deep integrity and session preflight consolidation;
- KG_FIRST material consumption/writeoff/costing compatibility;
- writeoff fault-path hardening;
- NDJSON growth observability;
- Hall settings/calibration safety flow;
- current Protocol/Web/safety CI recovery;
- current ESP32 compile/link recovery.

Reopen only after a concrete regression or explicit feature extension request.

## Deferred A — motor winding engineering knowledge base

Potential future work:

- collect verified manufacturer winding data in reviewed batches;
- distinguish factory/documented data from calculated/field-observed data;
- maintain provenance/confidence/source metadata;
- expand conductor/winding descriptors where needed for real comparison;
- build analogue suggestions only with explainable matching criteria.

Do not treat internet/community values as verified factory truth without provenance.

## Deferred B — aluminium-to-copper winding calculator

A future calculator may:

- convert aluminium conductor cross-section to copper equivalent;
- consider multiple parallel wires;
- consider only wire actually available in warehouse;
- evaluate slot-fill feasibility;
- present several candidate combinations;
- retain engineering assumptions/provenance;
- save result as a draft/calculation rather than silently altering motor truth.

Before production use, formulas and acceptance limits require explicit engineering validation.

## Deferred C — richer winding/motor comparison

Potential future schema may include:

```text
conductor material
diameter / parallel count
connection type
coil pitch
slot count
pole count
coil count
turns/program
winding type
lead/terminal scheme
```

This is needed to distinguish:

```text
same coil program
same winding
similar motor
confirmed analogue
```

Program similarity alone must never auto-merge motor identity/history.

## Deferred D — multi-currency policy

Do not add mixed-currency totals until policy exists for:

- repair currency;
- operation currency;
- exchange-rate source/date;
- historical rate snapshot;
- display and aggregation rules;
- fail-closed behavior for incompatible values.

Current historical costing must remain based on persisted operation snapshots.

## Deferred E — storage scaling beyond measured current strategy

Possible future options include segmentation/rotation/indexing/database migration, but only after device measurements show an actual bottleneck.

Do not implement:

```text
arbitrary rotation thresholds
destructive compaction
optimistic persistent caches
database migration
```

without measured size/latency/RAM evidence and a safe migration/rollback design.

Current reference:

```text
docs/85_NDJSON_PERFORMANCE_AND_ROTATION_STRATEGY.md
```

## Deferred F — update/maintenance enhancements

Potential later work:

- explicit firmware compatibility/version UI;
- carefully designed ESP32 OTA/update flow;
- safer packaged `/web` update workflow;
- maintenance export/log tooling;
- additional non-destructive module diagnostics.

Any firmware update feature must preserve recovery path and must never create remote physical START/SSR authority.

## Deferred G — additional hardware modules

Any new ESP32/Arduino module is a separate integration project. Use:

```text
docs/AI_AGENT/03_ADD_MODULE_PLAYBOOK.md
docs/HARDWARE_REFERENCE/
```

Before integration define power, logic voltage, pins/bus, ownership, initialization failure semantics, diagnostics and targeted hardware verification.

## Deferred H — broader destructive fault campaign

Destructive tests such as intentional filesystem corruption or power interruption may be useful, but only on disposable media/filesystem images.

Never perform destructive fault injection on the working production microSD.

## Safety boundaries for all future work

Never weaken:

- physical START only;
- no automatic repeat START;
- Arduino owns SSR;
- no auto-resume after reboot;
- RUN_COMPLETED does not auto-writeoff;
- exact source session/run provenance for manual consumption;
- exact spool provenance whenever a spool is used;
- operator-only fail-closed restore;
- no automatic production-data deletion.

## How to promote a deferred item

A deferred item becomes active only when the user explicitly requests it or current evidence makes it necessary. Then move the concrete next action into `06_ACTIVE_WORK_AND_NEXT_STEPS.md`; do not use this backlog directly as an execution queue.
