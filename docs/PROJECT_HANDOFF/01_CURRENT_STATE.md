# Текущее состояние CoilMaster

Дата обновления: **2026-08-26**  
Ветка: **`cmp-protocol-v1`**

## Source of truth

Working source only `cmp-protocol-v1`; `main` для исходников не использовать.

## Current phase

GREEN through checkpoint **139**. Atomic RUN_WIRE is the only current wire mutation path. Warehouse summary and the first finalization write-off coverage batch reuse authoritative movement validation, MaterialLedger public lookups are explicitly fail-closed, dead private ledger helpers are removed, and RepairCosting repair identity validation no longer uses an ambiguous bool wrapper.

## Latest GREEN state

```text
130 direct legacy Store mutation implementations removed
131 warehouse repair lookup -> explicit found
132 warehouse spool identity + wire catalogue -> explicit found/count
133 warehouse price public read -> explicit configured
134 warehouse summary -> one authoritative movement validation/aggregation pass
135 MaterialLedger public repair/state/currency -> explicit found
136 dead adjustmentExists full-log helper removed
137 one-arg MaterialLedger repair + dead usageExists/restoreQuantity removed
138 RepairCosting one-arg repair wrapper removed; load() uses explicit found
139 first bounded finalization coverage batch fused with authoritative movement pairing/provenance audit
```

Production RUN_WIRE path remains:

```text
RUN_COMPLETED (evidence only)
-> explicit operator Material Request RUN_WIRE ISSUE
-> immutable movement
-> MaterialLedger RWI_TX usage
-> managed warehouse PENDING/CONFIRMED
```

Checkpoint 139 keeps the checkpoint-55 fixed 32-target RAM bound. For the first winding-history batch, exact run/spool coverage is collected during the same authoritative movement pass that validates PENDING/CONFIRMED pairing and global confirmed provenance uniqueness. Later history pages keep the bounded lightweight movement scan. The former standalone movement-integrity pre-scan is removed.

Latest verified checkpoint-139 evidence:

```text
0e4896e0139ac8b7f79effb02d644e42dd057d22  final source
1d78995c5cd203cbfadbaf67aa03b48a813a5ca0  final contract
ESP32 Build #1602   32979299677 / SUCCESS
CMP Tests #3635     32979340004 / SUCCESS
```

Checkpoint: `139_FINALIZATION_WRITEOFF_FIRST_BATCH_FUSED_AUDIT_2026-08-26.md`.

## Current NEXT

1. Continue bounded audit of growing append-only readers for concrete redundant full scans.
2. Reuse authoritative parsers/audits where possible; keep fixed-size RAM bounds.
3. Preserve single-pass costing ownership and Web HTTP preflight semantics.
4. No automatic production-data rotation/deletion/truncation and no premature DB migration.
5. Preserve historical recovery/history and atomic RUN_WIRE safety.

## Safety invariants

Physical START local-only; no auto-resume; Arduino owns SSR; ESP32/Web never controls SSR directly; `RUN_COMPLETED` never deducts material automatically; exact run/spool provenance remains mandatory; restore remains operator-only/transactional/fail-closed.

## Hardware acceptance

Full two-board E2E remains mandatory after software stabilization.
