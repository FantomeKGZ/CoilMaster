# Активная работа и следующие шаги

Дата обновления: **2026-08-27**  
Ветка: **`cmp-protocol-v1`**

## GREEN foundation through checkpoint 155; checkpoint 156 under ESP32 verification

```text
148 managed RUN_WIRE removes redundant spool pre-scan
149 spool/material bridge append -> one validated bridge-log pass
150 MaterialLedger confirmUsage two-pass retained as safety boundary
151 remaining append audit -> no safe same-ledger duplicate full scan
152 autonomous save -> one bounded-tail latest-event read
153 dead-helper linker audit -> NO-CHANGE; linker GC already strips them
154 autonomous task query parsed once per page
155 motor similarity candidate parsed once; each stored winding program parsed once
156 motor similarity Web handler reuses one coil_program request String
```

Production commits:

```text
1e831cde072f6c6152d10b7e71cb6a1e0f2a7b0e  checkpoint 152
a2f98cb377873d88d3fd103b6dfdfbabaf28ea65  checkpoint 154
415394d162de0f1c83e433cbbea3db94833b3162  checkpoint 155
78b41d38abdf89b9e72a02eea37edcd346c9610f  checkpoint 156
```

Latest direct verification:

```text
CMP Tests #3720    33042574144 / SUCCESS
ESP32 Build #1629  33042574134 / SUCCESS
CMP Tests #3721    33042622904 / SUCCESS
CMP Tests #3722    33042647618 / SUCCESS
CMP Tests #3723    33042699795 / SUCCESS
CMP Tests #3724    33042727200 / SUCCESS
CMP Tests #3725    33043013899 / SUCCESS  (checkpoint 155 production commit)
ESP32 Build #1630  33043013882 / SUCCESS  (checkpoint 155 production commit)
CMP Tests #3729    33043230389 / SUCCESS  (checkpoint 156 production commit)
ESP32 Build #1631  33043230391 / in_progress at last direct check
```

CMP host audit remains 69 mandatory steps.

## Checkpoint 155

`RepairRegistry::appendSimilarMotorsJson()` previously validated each persisted `coil_program` and then reparsed it in `equivalent()`. The unchanged candidate program was also reparsed for every motor. The current path parses the candidate once into fixed `uint16_t[10]`, parses each stored program once, and compares count/turn values directly. Parser grammar/limits, malformed-record fail-closed behavior, identity scoring, counts, truncation and JSON output are unchanged.

## Checkpoint 156

`MotorSimilarityWeb::handleLookup()` previously called `m_server.arg("coil_program")` repeatedly for empty checking, parser validation and candidate construction. It now checks argument presence first, fetches the value once into `const String coilProgram`, then reuses that same value. Existing HTTP behavior remains unchanged:

- missing/empty program -> `400 coil_program_required`;
- malformed program -> `400 invalid_coil_program`;
- registry failure -> `500 similarity_lookup_failed`.

The larger repair-page/status candidate was audited but intentionally not changed. Repairs may close out of `repair_id` order, so a lockstep `repairs.ndjson` + `repair-status.ndjson` stream is not valid. Collapsing repeated batch status scans into one request-wide scan would require retaining an unbounded candidate set when status-filter matches are sparse, violating the fixed-memory rule.

## Current active queue — checkpoint 156 verification / 157

1. Confirm ESP32 Build #1631 on `78b41d38...`; do not call checkpoint 156 GREEN before direct success.
2. After #1631 SUCCESS, mark checkpoint 156 GREEN and begin checkpoint 157 from the then-current `cmp-protocol-v1` HEAD.
3. Continue only with measurable runtime/storage/flash candidates; do not force cosmetic refactors.
4. Prefer same-operation duplicate parsing/read elimination or bounded fixed-memory aggregate/tail techniques where historical integrity semantics remain intact.
5. Do not replace authoritative historical integrity scans with tail-only shortcuts.
6. Keep MaterialLedger `confirmUsage()` two-pass unless a future common writer lock spans preflight through atomic swap with equivalent recovery proof.
7. Keep separate-ledger validation for different integrity domains or distinct mutation phases.
8. Keep fixed-size RAM; no whole-file buffering or unbounded vectors.
9. Preserve HTTP preflight semantics, mutation-time TOCTOU validation, exact-spool provenance, deterministic recovery and all existing safety invariants.
10. No automatic production-data rotation/deletion/truncation and no premature DB/index migration.

## Safety invariants

No automatic physical START/repeat START/resume/writeoff. Arduino owns SSR. ESP32/Web never controls SSR directly. Lost ACK never proves idle. Exact Material Request/item/spool/session/run provenance remains mandatory. Historical recovery stays deterministic. Restore stays operator-only and fail-closed.

## Hardware acceptance

Full two-board E2E remains mandatory after software stabilization.
