# Checkpoint 131 — fail-closed warehouse repair lookup

Дата: **2026-08-26**  
Ветка: **`cmp-protocol-v1`**

## Status

**GREEN (software / CI).**

## Change

Removed the ambiguous convenience overload:

```text
bool WarehouseStore::repairExists(uint32_t repairId) const;
```

The only remaining API is:

```text
bool WarehouseStore::repairExists(uint32_t repairId, bool& found) const;
```

This preserves two independent signals:

```text
return value -> storage/schema/read operation succeeded
found        -> exact repair_id exists
```

Therefore callers cannot silently collapse `read/integrity failure` into `not found`.

## Runtime impact

Current warehouse Web already uses the two-result form and can distinguish unavailable/read failure from a genuine missing repair. No production mutation or persistence format changed.

## Commits

```text
6ccdec084001faabf25eaf5b28177d8f7e89a7d5  remove ambiguous header overload
279fc281b42a559f89f911e9f0b2758ccd02e8ff  keep fail-closed implementation only
d848ce45eb35c7f2bba817d6de1efd0c4f4a02bd  mandatory fail-closed lookup contract
```

## Verified CI

```text
ESP32 Build #1576
run 32964609675
SUCCESS

CMP Protocol Tests #3570
run 32964670388
SUCCESS
```

All mandatory host audit steps in #3570 completed SUCCESS.

## Safety preserved

- no material mutation semantics changed;
- `RUN_COMPLETED` remains non-mutating;
- atomic RUN_WIRE remains the only current production wire mutation path;
- historical recovery/history remain unchanged;
- no automatic START/resume/writeoff;
- no new full-log scan was introduced.

## NEXT

Continue auditing ambiguous WarehouseStore convenience overloads. Remove/narrow only when compilation proves there are no required callers and the remaining API preserves explicit unavailable/not-found/configured distinctions. Keep historical recovery helpers/codecs intact.
