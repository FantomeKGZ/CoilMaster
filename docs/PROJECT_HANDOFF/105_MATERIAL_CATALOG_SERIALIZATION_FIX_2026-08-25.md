# CoilMaster — MaterialLedger catalog serialization fix

Дата: **2026-08-25**  
Ветка: **`cmp-protocol-v1`**  
Статус: **GREEN**

## Finding

При подготовке existing `MaterialLedger` к reuse как generic warehouse item catalog обнаружен production defect в `MaterialLedger::addMaterial()`.

До исправления unit JSON field не закрывался перед `stock_quantity_milli`:

```text
"unit":"PIECE,"stock_quantity_milli":...
```

Это могло создавать malformed `/data/materials/materials.ndjson` при `POST /api/materials`.

## Fix

Исправлена сериализация на canonical form:

```text
"unit":"PIECE","stock_quantity_milli":...
```

Поведение stock/cost/recovery не менялось.

## Commits

```text
1850e5c70e521311f97b89cd6f0dc66dd114e4c6  add regression
1ef8027837716907873f60e6238d7665b6a95363  fix MaterialLedger unit JSON field
3797f8edb288bce3b18b7767d3937952f71d6f81  wire permanent CMP regression
4c2cd989e9ff88a511282143b565ac2d77a9d57c  firmware verification head + quantity-scale contract note
```

Temporary patch helper was returned to inert/manual-only state in:

```text
756a97c4e4c2fd5d8030b8433f92f9a534067e80
```

## Permanent regression

```text
Tests/Web/check_material_catalog_serialization.js
```

It protects:

- closing quote after unit value;
- all existing unit enum mappings;
- structural validation presence;
- no regression back to malformed stock field concatenation.

## Verified CI

```text
CMP Protocol Tests run 32856412170 / SUCCESS
ESP32 Build run 32856412196 / SUCCESS
```

Verification head:

```text
4c2cd989e9ff88a511282143b565ac2d77a9d57c
```

## Architectural decision

Do **not** create a second generic warehouse catalog. Existing `/data/materials/materials.ndjson` / `MaterialLedger` remains the authoritative generic material/item catalog.

Its quantity model is integer `stock_quantity_milli` and `price_per_unit_minor`. Material Request must adapt to it explicitly rather than silently reinterpret units.

## Next

Implement a canonical adapter between Material Request units and MaterialLedger units/cost scale, then expose exact item lookup/state required by explicit operator ISSUE/RETURN/CORRECTION.

No automatic `RUN_COMPLETED` material deduction is introduced by this block.
