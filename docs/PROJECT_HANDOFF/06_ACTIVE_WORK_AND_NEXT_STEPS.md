# Активная работа и следующие шаги

Дата обновления: **2026-08-27**  
Ветка: **`cmp-protocol-v1`**

## GREEN foundation through checkpoint 140

```text
97-116 CRM / Material Request / Motor / Client / Cash
117-127 exact-spool bridge + atomic RUN_WIRE + accounting/provenance convergence
128-130 obsolete direct writeoff API/types/implementations removed
131-134 warehouse fail-closed reads + single-pass movement summary
135 MaterialLedger public repair/state/currency lookups require explicit found
136 dead adjustmentExists full-log helper removed
137 private repair wrapper + dead usageExists/restoreQuantity helpers removed
138 RepairCosting repair lookup wrapper removed; load() explicit found
139 first bounded finalization coverage batch shares authoritative movement pairing/provenance pass
140 winding-session preflight kept only for mutation-sensitive spool-selection directory
```

Verified checkpoint-140 evidence:

```text
1584672e49288334da531235e3bec9f6a691fc7f
0e3786c2894cd5b078645ae24ed5ceb3975cb4ea
ESP32 Build #1606  32981707495 / SUCCESS
CMP Tests #3644    32981785788 / SUCCESS
```

## Pending checkpoints 141-142

141 CRM client/motor existence lookups use explicit `found`; final source `35ba207a678547189a550aea9257ed1660d9853a` is verified by ESP32 `#1612` and CMP `#3656`, while the final contract run `#3657` ended in GitHub Actions `startup_failure` before any job was created.

142 moves unused `JobSnapshotStore::exists(sessionId)` out of the public API; source commit `7a2639832f1c2c0225fbe0de0a6d817bfc6ba622`. Its CMP `#3658` and ESP32 `#1613` runs are stuck before job creation (`jobs=[]`) and cannot be force-cancelled because GitHub returns HTTP 409. These infrastructure-blocked runs are neither GREEN nor RED evidence.

Do not promote 141/142 into the canonical GREEN foundation until a later normal CMP/ESP32 execution validates a commit containing those changes.

## Current production boundary

```text
RUN_COMPLETED -> evidence only
explicit operator RUN_WIRE ISSUE
-> one immutable Material Request movement
-> one MaterialLedger RWI_TX usage
-> managed physical warehouse PENDING/CONFIRMED
```

Finalization write-off coverage remains fixed at 32 targets per page. Winding-session persistence no longer pre-scans snapshot/state directories before `begin()` because those stores do not recover temp files; their normal content passes validate them once. Spool-selection retains read-only preflight because its `begin()` may promote validated temp evidence.

## Current active queue — bounded growing-file optimization

1. Highest-value next performance block: `WindingJournal` currently makes multiple full `events.ndjson` passes during one save/session-state operation (duplicate/start/active/highest/completed evidence). Design a single authoritative bounded session-analysis pass when CI execution is available again.
2. Do not add unbounded vectors or whole-file buffering; session evidence must remain fixed-size/streamed.
3. Until GitHub Actions can create jobs again, limit writes to low-risk API visibility/dead-helper cleanup and read-only audits; do not perform a large critical journal rewrite without compile/host proof.
4. Audit remaining append-only readers for concrete duplicate full scans of the same authoritative file.
5. Prefer returning bounded evidence/aggregates from an existing authoritative parser rather than adding parallel parsers.
6. Preserve Web HTTP preflight semantics, mutation-time TOCTOU validation, costing ownership and deterministic recovery.
7. Keep diagnostics read-only; automatic cleanup/rotation/deletion remains disabled; no premature DB migration.
8. Continue software optimization before mandatory final two-board hardware E2E.

## Safety invariants

No automatic physical START/repeat START/resume/writeoff; Arduino owns SSR; lost ACK never proves idle; exact Material Request/item/spool/session/run provenance stays mandatory; historical recovery remains deterministic; restore stays operator-only and fail-closed; no automatic production-data deletion/truncation.
