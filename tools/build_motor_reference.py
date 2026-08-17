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


def source_winding_sets(record: dict) -> list[dict]:
    for key in ("winding_sets", "winding_sets_source"):
        sets = record.get(key)
        if isinstance(sets, list) and sets:
            return [item for item in sets if isinstance(item, dict)]
    return []


def winding_set_value(item: dict, field: str):
    candidates = {
        "wire": ("wire_diameter_source", "wire_diameter_mm", "d_over_dprime_source", "conductor_source"),
        "source_n": ("source_n_value", "sp_source", "source_sp_value"),
        "pitch": ("winding_pitch_source", "pitch_source"),
        "power": ("power_kw_source", "power_kw"),
        "branches": ("parallel_branches_source",),
    }
    for key in candidates.get(field, (field,)):
        value = item.get(key)
        if value is not None:
            return value
    return None


def winding_set_text(record: dict, field: str) -> str | None:
    parts = []
    for item in source_winding_sets(record):
        value = winding_set_value(item, field)
        if value is None:
            continue
        label = item.get("label") or item.get("name") or item.get("part") or item.get("poles") or "winding"
        parts.append(f"{label}:{value}")
    return " | ".join(parts) if parts else None


def source_identity(record: dict) -> tuple[str | None, list[str]]:
    model_value = record.get("model")
    model = str(model_value).strip() if model_value is not None else ""
    aliases = record.get("aliases_in_source")
    normalized_aliases = []
    if isinstance(aliases, list):
        normalized_aliases.extend(str(alias).strip() for alias in aliases if alias is not None and str(alias).strip())

    # Some source tables print equivalent designations in one cell as
    # "PRIMARY / ALIAS". Only split a spaced slash so multispeed model
    # names such as AIR90L8/4 remain untouched.
    if " / " in model:
        parts = [part.strip() for part in model.split(" / ") if part.strip()]
        if parts:
            model = parts[0]
            normalized_aliases.extend(parts[1:])

    deduped_aliases = []
    seen = {model}
    for alias in normalized_aliases:
        if alias not in seen:
            deduped_aliases.append(alias)
            seen.add(alias)
    return model or None, deduped_aliases


def flatten(path: Path, doc: dict, record: dict) -> dict:
    wire = record.get("wire_diameter_source")
    if wire is None:
        wire = record.get("wire_diameter_mm")
    if wire is None:
        wire = record.get("d_over_dprime_source")
    if wire is None:
        wire = record.get("conductor_source")
    if wire is None:
        wire = winding_set_text(record, "wire")

    power = record.get("power_kw")
    if power is None:
        power = record.get("power_kw_source")
    if power is None:
        power = winding_set_text(record, "power")

    source_n = record.get("source_n_value")
    if source_n is None:
        source_n = record.get("sp_source")
    if source_n is None:
        source_n = record.get("source_sp_value")
    if source_n is None:
        source_n = winding_set_text(record, "source_n")

    pitch = record.get("winding_pitch_source")
    if pitch is None:
        pitch = record.get("pitch_source")
    if pitch is None:
        pitch = winding_set_text(record, "pitch")

    branches = record.get("parallel_branches_source")
    if branches is None:
        branches = winding_set_text(record, "branches")

    current = record.get("current_a")
    if current is None:
        current = record.get("current_a_source")
    voltage = record.get("voltage_v")
    if voltage is None:
        voltage = record.get("voltage_source")

    model, aliases = source_identity(record)
    sets = source_winding_sets(record)
    source_file = str(path.relative_to(ROOT)).replace("\\", "/")
    return {
        "reference_only": True,
        "series": doc.get("series") or path.parent.name,
        "manufacturer": doc.get("series") or path.parent.name,
        "speed_group_rpm": speed_group(record),
        "rated_speed_rpm": record.get("rated_speed_rpm"),
        "slot_count": record.get("slot_count"),
        "model": model,
        "aliases": aliases,
        "variant_key": record.get("variant_key"),
        "power_kw": power,
        "current_a": current,
        "voltage_v": voltage,
        "connection": record.get("connection_source"),
        "source_n": str(source_n if source_n is not None else ""),
        "wire": str(wire if wire is not None else ""),
        "pitch": str(pitch if pitch is not None else ""),
        "bore_mm": record.get("stator_bore_mm"),
        "core_length_mm": core_length(record),
        "parallel_branches": branches,
        "winding_sets": sets or None,
        "status": status(record),
        "source_id": record.get("source_id"),
        "source_title": source_label(doc, record.get("source_id", "")),
        "source_file": source_file,
        "source_files": [source_file],
    }


def nonempty(value) -> bool:
    if value is None:
        return False
    if isinstance(value, str):
        return bool(value.strip())
    if isinstance(value, (list, dict, tuple, set)):
        return bool(value)
    return True


def merge_record(target: dict, supplement: dict) -> None:
    for alias in supplement.get("aliases") or []:
        if alias not in target["aliases"] and alias != target.get("model"):
            target["aliases"].append(alias)

    overwrite_fields = (
        "rated_speed_rpm", "slot_count", "power_kw", "current_a", "voltage_v", "connection",
        "source_n", "wire", "pitch", "bore_mm", "core_length_mm", "parallel_branches", "winding_sets",
    )
    for field in overwrite_fields:
        value = supplement.get(field)
        if nonempty(value):
            target[field] = value

    if target.get("status") == "REVIEW_REQUIRED" or supplement.get("status") == "REVIEW_REQUIRED":
        target["status"] = "REVIEW_REQUIRED"

    for source_file in supplement.get("source_files") or []:
        if source_file not in target["source_files"]:
            target["source_files"].append(source_file)


def merge_key(record: dict) -> tuple[str, str | None, str | None]:
    return (str(record.get("series") or ""), record.get("model"), record.get("variant_key"))


def main() -> None:
    documents: list[tuple[Path, dict]] = []
    for path in sorted(SOURCE_ROOT.rglob("*.source.json")):
        documents.append((path, json.loads(path.read_text(encoding="utf-8"))))

    records: list[dict] = []
    for path, doc in documents:
        if doc.get("merge_only") is True:
            continue
        for record in doc.get("records", []):
            records.append(flatten(path, doc, record))

    for path, doc in documents:
        if doc.get("merge_only") is not True:
            continue
        for raw in doc.get("records", []):
            supplement = flatten(path, doc, raw)
            key = merge_key(supplement)
            candidates = [record for record in records if merge_key(record) == key]
            if len(candidates) != 1:
                raise ValueError(
                    f"merge_only record {path.relative_to(ROOT)} {key!r} matched {len(candidates)} base records; expected exactly 1"
                )
            merge_record(candidates[0], supplement)

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
