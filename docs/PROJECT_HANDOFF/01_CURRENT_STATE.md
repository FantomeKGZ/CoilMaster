# Текущее состояние CoilMaster

Дата обновления: **2026-08-26**  
Ветка: **`cmp-protocol-v1`**

## Source of truth / stable baseline

Working source-of-truth only `cmp-protocol-v1`. `main` не использовать как source.

```text
stable pre-CRM: 449570d47649d5f6336a31ee3eed491256e0fb1a
main -> same commit
stable-2026-08-25-pre-crm-redesign -> same commit
```

## Current phase

Workshop Web/CRM redesign is GREEN through Cash Web. The coordinated wire-accounting migration is now in progress.

## Latest GREEN migration state

Checkpoint 118 established append-only physical-spool ↔ generic-material bridge persistence:

```text
spool_id <-> warehouse_item_id + CU/AL + diameter
/data/warehouse/spool-material-bridges.ndjson
```

Checkpoint 119 made MaterialLedger wire identity authoritative without breaking generic materials:

```text
optional pair on /data/materials/materials.ndjson
wire_type = CU | AL
diameter_hundredths_mm = exact diameter
wire metadata requires unit = GRAM
```

Properties:
- legacy/generic material records may omit both metadata fields;
- one-sided/invalid wire metadata fails closed;
- active material lookup returns structured metadata;
- swap/recovery and list validation preserve the same contract;
- bridge integrity now requires exact `GRAM + CU/AL + diameter` match with MaterialLedger;
- no runtime bridge writer exists yet;
- existing exact-spool writeoff/finalization remains authoritative.

Latest evidence:

```text
CMP Protocol Tests 32941574082 / SUCCESS
ESP32 Build         32941574080 / SUCCESS
```

## Current NEXT

Add explicit operator-only bridge creation. It must preflight exact active physical spool + exact active MaterialLedger item, require matching `CU|AL + diameter`, reject an already bridged spool, and append identity evidence only. It must not mutate stock or relax current exact-spool writeoff/finalization.

After that, migrate run-linked wire accounting coherently toward explicit Material Request `RUN_WIRE` ISSUE while preserving exact `source_session_id + source_run_id` and physical spool provenance.

## Safety invariants

Never weaken:

- physical START local-only;
- no automatic START between repeats;
- no auto-resume after reboot;
- Arduino owns SSR;
- ESP32/Web never directly controls SSR;
- lost ACK/timeout never proves Arduino idle;
- final repeat cannot auto-reopen;
- `RUN_COMPLETED` never automatically deducts material;
- warehouse ISSUE requires explicit operator action;
- cash operations never trigger machine or warehouse mutation;
- run-linked wire movement preserves exact run provenance;
- cancellation/operator abort preserves immutable evidence;
- restore operator-only, transactional, fail-closed;
- no automatic production-data deletion/truncation.

## Hardware acceptance

Not complete. Full two-board E2E remains mandatory after CRM/material/writeoff contracts stabilize.
