# CoilMaster motor and winding import format

Import UI:

- desktop: `/desktop/motor-import.html`
- mobile: `/mobile/motor-import.html`

The input is a UTF-8 JSON array containing 1–50 objects. Preview performs local validation and calls the existing similarity endpoint for every valid object. Preview never writes to microSD. Exact identity matches are unchecked by default. Only explicitly selected objects are submitted, one record at a time, to `POST /api/motors`.

Field names are strict: an unknown field is an error instead of being silently
ignored. Duplicate identities inside the same JSON package are rejected before
any write. After a successful row import, that row is disabled in the current
preview so the same button cannot submit it twice; failed rows remain available
for a deliberate retry.

## Required fields

| Field | Meaning |
|---|---|
| `name` | Human-readable motor name |
| `coil_program` | 1–10 winding values, each 1–9999, separated by `/` |
| `source_type` | `MANUFACTURER`, `TECHNICAL_REFERENCE`, `REPAIR_RECORD`, `CALCULATED`, or `UNVERIFIED` |
| `source_title` | Exact catalogue, manual, repair card, calculation, or page title |
| `confidence` | `VERIFIED`, `CORROBORATED`, `CALCULATED`, or `UNVERIFIED` |

## Optional identity and electrical fields

`model`, `manufacturer`, `tags`, `comment`, `rated_power_w`, `rated_voltage_v`, `rated_current_ma`, `rated_speed_rpm`, `frequency_hz`, `phases`, `connection`.

`connection` is one of `Y`, `DELTA`, or `Y/DELTA`.

## Optional winding and geometry fields

`slot_count`, `pole_count`, `coil_pitch`, `turns_per_coil`, `wire_diameter_hundredths_mm`, `parallel_strands`, `wire_material`, `winding_type`, `stator_bore_mm`, `stator_core_length_mm`.

`wire_material` is `CU` or `AL`. Wire diameter is stored in hundredths of a millimetre: 0.95 mm is `95`.

## Source fields

`source_url`, `source_retrieved_at`, and `calculated_fields` are optional. Use an ISO date beginning with `YYYY-MM-DD`. Set `calculated_fields: true` whenever at least one imported winding value was calculated rather than copied from the cited source.

`source_retrieved_at`, when present, must be a real calendar date in exact
`YYYY-MM-DD` form (years 2000–2199). `source_url`, when present, must use HTTP or
HTTPS. `CALCULATED` in either `source_type` or `confidence` requires
`calculated_fields: true`; records without a calculated classification require
it to be false or omitted. The firmware repeats these provenance and text-size
checks before appending the NDJSON record.

Confidence rules:

- `VERIFIED`: direct manufacturer document or physically verified original winding record;
- `CORROBORATED`: consistent data from at least two independent credible sources;
- `CALCULATED`: one or more essential winding values were derived;
- `UNVERIFIED`: forum, advertisement, photograph inference, or single uncertain source.

Never relabel calculated or unverified data as manufacturer data. Different voltage, core length, slot count, pole count, or connection means a separate variant unless equivalence is proven.

See `docs/examples/motor-import.example.json`.
