# Активная работа и следующие шаги

Дата обновления: **2026-08-27**  
Ветка: **`cmp-protocol-v1`**

## GREEN foundation through checkpoint 145

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
141 CRM client/motor existence lookups use explicit found across registry/Web/cash/intake
142 unused JobSnapshotStore exists helper removed from public API
143 autonomous assignment public API narrowed to assignMotorChecked result semantics
144 WindingJournal runtime save/state evidence shares one streamed journal analysis pass
145 WindingJournal boot schema/context validation shares one combined pass
```

Verified latest evidence:

```text
bbbc53d96e535204e38db7da0fc79f872dd5a19a
1760a447ffd216976b844a51adb055a5701f16ff
ESP32 Build #1620  33035508401 / SUCCESS
CMP Tests #3681    33035532132 / SUCCESS
```

Intermediate CMP `#3680` was only the stale textual contract before the checkpoint-145 contract update.

## Current production boundary

```text
RUN_COMPLETED -> evidence only
explicit operator RUN_WIRE ISSUE
-> one immutable Material Request movement
-> one MaterialLedger RWI_TX usage
-> managed physical warehouse PENDING/CONFIRMED
```

After checkpoints 144-145, `CM_WindingJournal.cpp` has exactly two full reads of the growing event journal: one combined boot schema/context validation pass and one runtime session-analysis pass. Exact replay ordering, immutable schema-2 context checks, active-run pairing, monotonic START ids and completed-run evidence remain fail-closed.

## Current active queue — bounded growing-file optimization

1. Continue auditing other append-only runtime stores for concrete repeated full scans of the same file.
2. Prioritize frequent mutation/read paths over occasional operator-only scans. Autonomous archive replay/start checks are already bounded tail reads (<=512 bytes), while assignment validation intentionally scans separate events and assignment ledgers.
3. Audit CRM/cash/material-request append-only mutation paths for avoidable same-file `exists/next-id/validate` rescans.
4. Prefer authoritative parser/audit reuse over parallel custom parsers.
5. Keep fixed-size RAM bounds; no whole-file buffering or unbounded vectors.
6. Preserve Web HTTP preflight semantics, mutation-time TOCTOU validation, costing ownership and deterministic recovery.
7. Keep diagnostics read-only; no automatic cleanup/rotation/deletion/truncation and no premature DB migration.
8. Continue software optimization before mandatory final two-board hardware E2E.

## Safety invariants

No automatic physical START/repeat START/resume/writeoff; Arduino owns SSR; lost ACK never proves idle; exact Material Request/item/spool/session/run provenance stays mandatory; historical recovery remains deterministic; restore stays operator-only and fail-closed; no automatic production-data deletion/truncation.
