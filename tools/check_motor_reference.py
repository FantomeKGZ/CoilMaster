#!/usr/bin/env python3
"""Fail when the committed winding-reference index is stale or unsafe.

This check is read-only. It compares the committed static index against the current
*.source.json catalogue without creating working motor records or coil_program data.
"""
from __future__ import annotations

import importlib.util
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
GENERATOR = ROOT / "tools" / "build_motor_reference.py"
INDEX = ROOT / "firmware" / "esp32" / "web" / "reference" / "motor-reference.json"


def load_generator():
    spec = importlib.util.spec_from_file_location("cm_motor_reference_builder", GENERATOR)
    if spec is None or spec.loader is None:
        raise RuntimeError("generator_load_failed")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def expected_records(builder) -> list[dict]:
    documents: list[tuple[Path, dict]] = []
    for path in sorted(builder.SOURCE_ROOT.rglob("*.source.json")):
        documents.append((path, json.loads(path.read_text(encoding="utf-8"))))

    records: list[dict] = []
    for path, doc in documents:
        if doc.get("merge_only") is True:
            continue
        for record in doc.get("records", []):
            records.append(builder.flatten(path, doc, record))

    for path, doc in documents:
        if doc.get("merge_only") is not True:
            continue
        for raw in doc.get("records", []):
            supplement = builder.flatten(path, doc, raw)
            key = builder.merge_key(supplement)
            candidates = [record for record in records if builder.merge_key(record) == key]
            if len(candidates) != 1:
                raise ValueError(
                    f"merge_only record {path.relative_to(ROOT)} {key!r} matched {len(candidates)} base records; expected exactly 1"
                )
            builder.merge_record(candidates[0], supplement)

    records.sort(key=lambda x: (str(x.get("series")), -(x.get("speed_group_rpm") or 0), x.get("slot_count") or 0, str(x.get("model"))))
    return records


def stable_identity(record: dict) -> tuple:
    return (
        record.get("source_file"),
        record.get("variant_key"),
        record.get("model"),
        record.get("slot_count"),
        record.get("source_n"),
        record.get("wire"),
        record.get("pitch"),
        tuple(record.get("source_files") or []),
    )


def main() -> int:
    if not INDEX.is_file():
        print(f"missing reference index: {INDEX.relative_to(ROOT)}", file=sys.stderr)
        return 1

    builder = load_generator()
    expected = expected_records(builder)
    actual_doc = json.loads(INDEX.read_text(encoding="utf-8"))
    actual = actual_doc.get("records")

    if actual_doc.get("reference_only") is not True or not isinstance(actual, list):
        print("reference index must be reference_only=true with a records array", file=sys.stderr)
        return 1

    if any(record.get("coil_program") for record in actual if isinstance(record, dict)):
        print("reference index must never contain coil_program", file=sys.stderr)
        return 1

    expected_ids = [stable_identity(record) for record in expected]
    actual_ids = [stable_identity(record) for record in actual if isinstance(record, dict)]

    errors: list[str] = []
    if len(actual) != len(expected):
        errors.append(f"record_count stale: committed={len(actual)} expected={len(expected)}")
    if actual_ids != expected_ids:
        missing = [item for item in expected_ids if item not in set(actual_ids)]
        extra = [item for item in actual_ids if item not in set(expected_ids)]
        errors.append(f"content stale: missing={len(missing)} extra={len(extra)} or ordering/fields changed")
        for item in missing[:5]:
            errors.append(f"  missing: {item}")
        for item in extra[:5]:
            errors.append(f"  extra: {item}")

    declared = actual_doc.get("record_count")
    if declared is not None and declared != len(actual):
        errors.append(f"record_count metadata mismatch: declared={declared} actual={len(actual)}")

    if errors:
        print("winding reference index is stale; run tools/build_motor_reference.py", file=sys.stderr)
        for error in errors:
            print(error, file=sys.stderr)
        return 1

    print(f"winding reference index is current: {len(actual)} records")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
