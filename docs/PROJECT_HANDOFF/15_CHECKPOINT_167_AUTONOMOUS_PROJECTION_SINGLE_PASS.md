# Checkpoint 167 — autonomous canonical projection preflight single-pass

Date: **2026-08-29**  
Branch: **`arduino-ru-lcd-experiment`**  
Production `cmp-protocol-v1` remains unchanged at `28c7917a906bc9b15736369e8986d0e0c354ab8c`.

## Status

**GREEN**

## Confirmed repeated scan

`AutonomousWindingArchive::ensureCanonicalProjection()` previously performed two consecutive read-only full passes over the same growing journal:

```text
/data/workshop/motor-winding-versions.ndjson
```

The first pass, `findAutonomousProjection()`, proved exact retry provenance:

```text
source_autonomous_session_id + source_autonomous_run_id + source_autonomous_role
```

If no exact projection existed, the second pass, `loadLatestByMotor()`, immediately rescanned the same journal to obtain the latest complete winding state for the target motor.

A later `MotorWindingVersionStore::append()` pass is a different phase: it performs authoritative mutation-time journal validation and `nextVersionId()` allocation immediately before append. That scan is intentionally retained.

## Change

Added bounded streaming API:

```cpp
MotorWindingVersionStore::analyzeAutonomousProjection(...)
```

It performs the two read-only preflight proofs in one authoritative full pass:

- validates strict `winding_version_id` ordering;
- validates every optional autonomous provenance triplet exactly as before;
- detects at most one exact `session_id + run_id + role` projection;
- captures its exact projected motor/version ids;
- parses and retains the latest complete version for the requested target motor;
- preserves fail-closed behavior for malformed/duplicate provenance;
- uses constant/bounded state and does not buffer the journal.

`ensureCanonicalProjection()` now calls only this fused preflight API. Existing standalone `findAutonomousProjection()` and `loadLatestByMotor()` remain available for callers that need their generic self-validating behavior.

## Mutation boundary retained

The final canonical append still calls:

```cpp
m_motorWindingVersions.append(next, appendedVersionId)
```

and `append()` still executes its own `nextVersionId()` authoritative journal scan. This intentionally preserves mutation-time allocator/integrity revalidation and avoids reusing stale preflight state across the mutation boundary.

No persistent cache, index, database, whole-file buffer, automatic rotation, truncation or history rewrite was added.

## Behavioral invariants unchanged

- identical autonomous retry remains idempotent by exact `session_id + run_id + role`;
- retry against another motor remains invalid;
- `STARTING` still requires an existing `WORKING` winding;
- occupied target role still requires explicit replacement;
- replacement still appends a new canonical version;
- the complete untargeted role is preserved;
- canonical provenance remains immutable;
- no physical RUN evidence is copied or rewritten;
- no automatic physical START or reboot resume;
- Arduino remains sole SSR owner;
- `RUN_COMPLETED` never causes automatic material writeoff;
- RUN_WIRE writeoff remains manual with exact spool/session/run provenance.

## Commits

```text
c38185f66c7b47e31321431d4c05ee1a5ba7d1f0  expose fused store API
d982e8f06ff8a1290e6a0c766f41811851d53b7d  implement one-pass autonomous projection analysis
5f6f5962fdfca23e5252af1d6250b994325b4183  switch projection preflight to fused API
14636b219142548f714172aa42143bcf1b2888d3  lock regression contract
```

## Verified CI

Runtime/code head `5f6f5962fdfca23e5252af1d6250b994325b4183`:

```text
CMP Protocol Tests #3989  run 33262503394 / SUCCESS
ESP32 Build #1765         run 33262503303 / SUCCESS
Arduino RU LCD #189       run 33262503311 / SUCCESS
```

Regression contract head `14636b219142548f714172aa42143bcf1b2888d3`:

```text
CMP Protocol Tests #3990  run 33262566240 / SUCCESS
```

## NEXT

Continue the experiment-side repeated-scan audit only where two or more scans belong to the same read-only/preflight phase and can be fused without removing mutation/recovery/TOCTOU validation. Prefer existing bounded streaming primitives and classify candidates NO-CHANGE when the second pass is an integrity boundary.
