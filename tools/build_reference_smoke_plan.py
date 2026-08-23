#!/usr/bin/env python3
"""Generate a bounded risk-based physical smoke plan for a built reference site."""
from __future__ import annotations

import argparse
import html
import json
import re
from pathlib import Path
from urllib.parse import quote

LEGACY_SECTION_RE = re.compile(
    r'<section\s+class="cm-reference-card cm-reference-legacy-page">\s*(?P<body>.*?)\s*</section\s*>',
    re.I | re.S,
)
NON_VISIBLE_RE = re.compile(r"<(?:script|style)\b[^>]*>.*?</(?:script|style)\s*>", re.I | re.S)
COMMENT_RE = re.compile(r"<!--.*?-->", re.S)
TAG_RE = re.compile(r"<[^>]+>", re.S)


def page_metrics(path: Path) -> dict[str, int]:
    text = path.read_text(encoding="utf-8")
    match = LEGACY_SECTION_RE.search(text)
    body = match.group("body") if match else text
    visible = NON_VISIBLE_RE.sub(" ", body)
    visible = COMMENT_RE.sub(" ", visible)
    visible = TAG_RE.sub(" ", visible)
    visible = re.sub(r"\s+", " ", html.unescape(visible)).strip()
    return {
        "tables": len(re.findall(r"<table\b", body, re.I)),
        "images": len(re.findall(r"<img\b", body, re.I)),
        "visible_characters": len(visible),
    }


def metric_winner(rows: list[dict[str, object]], metric: str) -> dict[str, object]:
    return sorted(
        rows,
        key=lambda row: (-int(row[metric]), str(row["path"]).casefold()),
    )[0]


def build_plan(output: Path) -> dict[str, object]:
    catalog_path = output / "shared" / "catalog.json"
    catalog = json.loads(catalog_path.read_text(encoding="utf-8"))
    if not isinstance(catalog, list) or not catalog:
        raise ValueError(f"catalog must be a non-empty list: {catalog_path}")

    rows: list[dict[str, object]] = []
    for entry in catalog:
        relative = str(entry["path"])
        modes: dict[str, dict[str, int]] = {}
        for mode in ("desktop", "mobile"):
            if not entry.get(mode):
                continue
            page = output / mode / "pages" / relative
            if not page.is_file():
                raise ValueError(f"catalog smoke target missing: {page}")
            modes[mode] = page_metrics(page)
        if not modes:
            raise ValueError(f"catalog entry has no generated mode: {relative}")
        rows.append(
            {
                "path": relative,
                "title": str(entry.get("title") or relative),
                "depth": len(Path(relative).parts),
                "tables": max(item["tables"] for item in modes.values()),
                "images": max(item["images"] for item in modes.values()),
                "visible_characters": max(item["visible_characters"] for item in modes.values()),
                "modes": modes,
                "desktop": bool(entry.get("desktop")),
                "mobile": bool(entry.get("mobile")),
            }
        )

    selected: dict[str, dict[str, object]] = {}

    def add(row: dict[str, object], reason: str) -> None:
        key = str(row["path"]).casefold()
        if key not in selected:
            selected[key] = {**row, "reasons": []}
        selected[key]["reasons"].append(reason)

    preferred = next(
        (
            row
            for row in rows
            if Path(str(row["path"])).stem.casefold() in {"4a", "4а"}
        ),
        None,
    )
    if preferred:
        add(preferred, "known-series-4A")

    for metric, reason in (
        ("tables", "most-tables"),
        ("images", "most-images"),
        ("visible_characters", "largest-visible-content"),
        ("depth", "deepest-path"),
    ):
        add(metric_winner(rows, metric), reason)

    ranked = sorted(
        rows,
        key=lambda row: (
            -(
                int(row["tables"]) * 100000
                + int(row["images"]) * 1000
                + int(row["visible_characters"])
                + int(row["depth"]) * 10
            ),
            str(row["path"]).casefold(),
        ),
    )
    for row in ranked:
        if len(selected) >= 5:
            break
        add(row, "risk-ranked-fill")

    pages: list[dict[str, object]] = []
    for row in selected.values():
        relative = str(row["path"])
        encoded = quote(relative, safe="/")
        page = {
            "path": relative,
            "title": row["title"],
            "reasons": row["reasons"],
            "metrics_by_mode": row["modes"],
        }
        if row["desktop"]:
            page["desktop_url"] = f"/sites/reference/desktop/pages/{encoded}"
        if row["mobile"]:
            page["mobile_url"] = f"/sites/reference/mobile/pages/{encoded}"
        pages.append(page)

    return {
        "schema_version": 1,
        "catalog_entries": len(catalog),
        "selected_pages": len(pages),
        "checks": [
            "open desktop and mobile URL",
            "return to search and reopen result",
            "switch mode and keep the same page",
            "scroll every wide table by touch/keyboard",
            "confirm images load without internet",
            "confirm SD commit provenance is visible",
        ],
        "pages": pages,
    }


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", required=True, type=Path, help="Built sites/reference directory")
    parser.add_argument("--report", required=True, type=Path)
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    plan = build_plan(args.output.resolve())
    report = args.report.resolve()
    report.parent.mkdir(parents=True, exist_ok=True)
    report.write_text(
        json.dumps(plan, ensure_ascii=False, indent=2),
        encoding="utf-8",
        newline="\n",
    )
    print(
        f"reference smoke plan: {plan['selected_pages']} pages "
        f"from {plan['catalog_entries']} catalog entries -> {report}"
    )


if __name__ == "__main__":
    main()
