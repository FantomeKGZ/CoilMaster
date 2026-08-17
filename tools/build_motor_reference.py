#!/usr/bin/env python3
"""Build the read-only CoilMaster winding-reference index from *.source.json files.

This generator never creates coil_program and never promotes records into the working
motor database. It only flattens source-native catalogue records for the static UI.
"""
from __future__ import annotations

import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE_ROOT = ROOT / "data" / "motor_catalog"
OUTPUT = ROOT / "firmware" / "esp32" / "web" / "reference" / "motor-reference.json"
SYNCHRONOUS_GROUPS = (3000, 1500, 1000, 750, 600, 500, 375, 300)


def speed_group(record: dict) -> int | None:
    rated = record.get("rated_speed_rpm")
    if isinstance(rated, (int, float)) and rated > 0:
        return min(SYNCHRONOUS_GROUPS, key=lambda value: abs(value - rated))
    model = str(record.get("model", "")).upper().replace(" ", "")
    match = re.search(r"(?:^|[A-ZА-ЯМLSABВ])((?:10|12|2|4|6|8))(?:У\d|U\d|$)", model)
    if not match:
        match = re.search(r"(10|12|2|4|6|8)(?:У\d|U\d|$)", model)
    if not match:
        return None
    poles = int(match.group(1))
    return 6000 // poles if poles else None


def core_length(record: dict):
    value = record.get("stator_core_length_mm")
    if value is not None:
        return value
    choices = record.get("stator_core_length_by_housing_mm")
    if isinstance(choices, dict):
        values = [v for v in choices.values() if v is not None]
        if len(values) == 1:
            return values[0]
        if values:
            return "/".join(str(v) for v in values)
    return None


def status(record: dict) -> str:
    marker = str(record.get("normalization_status", ""))
    if any(word in marker for word in ("SUSPECT", "REVIEW", "CONFLICT", "COMPOUND", "MULTI_WINDING")):
        return "REVIEW_REQUIRED"
    return "REFERENCE"


def source_label(doc: dict, source_id: str) -> str:
    for source in doc.get("sources", []):
        if source.get("id") == source_id:
            return source.get("title") or source_id
    return source_id


def winding_set_text(record: dict, field: str) -> str | None:
    sets = record.get("winding_sets")
    if not isinstance(sets, list) or not sets:
        return None
    parts = []
    for item in sets:
        if not isinstance(item, dict):
            continue
        value = item.get(field)
        if value is None:
            continue
        label = item.get("label") or item.get("poles") or "winding"
        parts.append(f"{label}:{value}")
    return " | ".join(parts) if parts else None


def source_aliases(record: dict) -> list[str]:
    aliases = record.get("aliases_in_source")
    if not isinstance(aliases, list):
        return []
    return [str(alias) for alias in aliases if alias is not None and str(alias).strip()]


def flatten(path: Path, doc: dict, record: dict) -> dict:
    wire = record.get("wire_diameter_source")
    if wire is None:
        wire = record.get("wire_diameter_mm")
    if wire is None:
        wire = winding_set_text(record, "wire_diameter_mm")
    power = record.get("power_kw")
    if power is None:
        power = record.get("power_kw_source")
    if power is None:
        power = winding_set_text(record, "power_kw_source")
    source_n = record.get("source_n_value")
    if source_n is None:
        source_n = winding_set_text(record, "source_n_value")
    pitch = record.get("winding_pitch_source")
    if pitch is None:
        pitch = winding_set_text(record, "winding_pitch_source")
    branches = record.get("parallel_branches_source")
    if branches is None:
        branches = winding_set_text(record, "parallel_branches_source")
    return {
        "reference_only": True,
        "series": doc.get("series") or path.parent.name,
        "manufacturer": doc.get("series") or path.parent.name,
        "speed_group_rpm": speed_group(record),
        "rated_speed_rpm": record.get("rated_speed_rpm"),
        "slot_count": record.get("slot_count"),
        "model": record.get("model"),
        "aliases": source_aliases(record),
        "variant_key": record.get("variant_key"),
        "power_kw": power,
        "current_a": record.get("current_a"),
        "voltage_v": record.get("voltage_v"),
        "connection": record.get("connection_source"),
        "source_n": str(source_n if source_n is not None else ""),
        "wire": str(wire if wire is not None else ""),
        "pitch": str(pitch if pitch is not None else ""),
        "bore_mm": record.get("stator_bore_mm"),
        "core_length_mm": core_length(record),
        "parallel_branches": branches,
        "winding_sets": record.get("winding_sets"),
        "status": status(record),
        "source_id": record.get("source_id"),
        "source_title": source_label(doc, record.get("source_id", "")),
        "source_file": str(path.relative_to(ROOT)).replace("\\", "/"),
    }


def main() -> None:
    records: list[dict] = []
    for path in sorted(SOURCE_ROOT.rglob("*.source.json")):
        doc = json.loads(path.read_text(encoding="utf-8"))
        for record in doc.get("records", []):
            records.append(flatten(path, doc, record))
    records.sort(key=lambda x: (str(x.get("series")), -(x.get("speed_group_rpm") or 0), x.get("slot_count") or 0, str(x.get("model"))))
    payload = {
        "schema_version": 1,
        "reference_only": True,
        "generated_from": "data/motor_catalog/**/*.source.json",
        "record_count": len(records),
        "records": records,
    }
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_text(json.dumps(payload, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(f"wrote {len(records)} reference records to {OUTPUT.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
