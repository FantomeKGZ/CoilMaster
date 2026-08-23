#!/usr/bin/env python3
"""Report safe cleanup candidates in a generated legacy reference tree.

This tool never deletes or rewrites generated content. It separates legacy pages
without legacy incoming links from assets that are not referenced by generated
HTML, and reports byte-identical files as review candidates.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import re
from collections import defaultdict
from pathlib import Path
from urllib.parse import unquote, urlsplit

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_OUTPUT = ROOT / "firmware" / "esp32" / "web" / "sites" / "reference"
HTML_SUFFIXES = {".html", ".htm"}
ATTR_RE = re.compile(r"\b(?:href|src)\s*=\s*[\"'](?P<value>.*?)[\"']", re.I)
CSS_URL_RE = re.compile(r"\burl\(\s*[\"']?(?P<value>[^\"')]+)", re.I)
REFERENCE_PREFIX = "/sites/reference/"


def digest(path: Path) -> str:
    value = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            value.update(chunk)
    return value.hexdigest()


def decode_legacy_stylesheet(path: Path) -> str:
    data = path.read_bytes()
    for encoding in ("utf-8-sig", "cp1251"):
        try:
            return data.decode(encoding)
        except UnicodeDecodeError:
            continue
    raise AssertionError("cp1251 decodes every byte sequence")


def reference_path(value: str) -> str | None:
    parts = urlsplit(value)
    if parts.scheme or parts.netloc or not parts.path.startswith(REFERENCE_PREFIX):
        return None
    segments: list[str] = []
    for segment in unquote(parts.path).split("/"):
        if not segment or segment == ".":
            continue
        if segment == "..":
            if not segments:
                return None
            segments.pop()
        else:
            segments.append(segment)
    return "/".join(segments)


def resolved_reference_path(value: str, source: Path, output: Path) -> str | None:
    absolute = reference_path(value)
    if absolute is not None:
        return absolute

    parts = urlsplit(value)
    if parts.scheme or parts.netloc or not parts.path or parts.path.startswith("/"):
        return None
    relative = unquote(parts.path).replace("\\", "/")
    candidate = (source.parent / relative).resolve()
    try:
        output_relative = candidate.relative_to(output.resolve())
    except ValueError:
        return None
    return "sites/reference/" + output_relative.as_posix()


def files_below(root: Path) -> list[Path]:
    return sorted(path for path in root.rglob("*") if path.is_file()) if root.exists() else []


def human_bytes(size: int) -> str:
    value = float(size)
    for unit in ("B", "KiB", "MiB", "GiB"):
        if value < 1024 or unit == "GiB":
            return f"{value:.1f} {unit}"
        value /= 1024
    raise AssertionError("unreachable")


def audit(output: Path) -> dict[str, object]:
    generated_html = [
        path
        for mode in ("desktop", "mobile")
        for path in files_below(output / mode / "pages")
        if path.suffix.lower() in HTML_SUFFIXES
    ]
    entry_html = [
        path for mode in ("desktop", "mobile")
        if (path := output / mode / "index.html").is_file()
    ]
    all_html = generated_html + entry_html
    reference_styles = [path for path in files_below(output) if path.suffix.lower() == ".css"]

    referenced_files: set[str] = set()
    incoming: dict[str, int] = defaultdict(int)
    for page in all_html:
        text = page.read_text(encoding="utf-8")
        source_mode = page.relative_to(output).parts[0]
        source_url = "sites/reference/" + page.relative_to(output).as_posix()
        for match in ATTR_RE.finditer(text):
            target = resolved_reference_path(match.group("value"), page, output)
            if target is None:
                continue
            referenced_files.add(target)
            prefix = f"sites/reference/{source_mode}/pages/"
            if page in generated_html and target.startswith(prefix) and target != source_url:
                incoming[target] += 1
        for match in CSS_URL_RE.finditer(text):
            target = resolved_reference_path(match.group("value"), page, output)
            if target is not None:
                referenced_files.add(target)
    for stylesheet in reference_styles:
        text = decode_legacy_stylesheet(stylesheet)
        for match in CSS_URL_RE.finditer(text):
            target = resolved_reference_path(match.group("value"), stylesheet, output)
            if target is not None:
                referenced_files.add(target)

    no_legacy_incoming: dict[str, list[str]] = {}
    for mode in ("desktop", "mobile"):
        prefix = f"sites/reference/{mode}/pages/"
        candidates = []
        for page in generated_html:
            rel = page.relative_to(output).as_posix()
            if rel.startswith(f"{mode}/pages/"):
                url_path = "sites/reference/" + rel
                if incoming[url_path] == 0:
                    candidates.append(rel.removeprefix(f"{mode}/pages/"))
        no_legacy_incoming[mode] = candidates

    asset_files = [
        path
        for subtree in (
            output / "shared" / "assets",
            output / "desktop" / "assets",
            output / "mobile" / "assets",
        )
        for path in files_below(subtree)
    ]
    unreferenced_assets = [
        path for path in asset_files
        if "sites/reference/" + path.relative_to(output).as_posix() not in referenced_files
    ]

    unused_types: dict[str, dict[str, int]] = {}
    for path in unreferenced_assets:
        kind = path.suffix.lower() or "<no extension>"
        values = unused_types.setdefault(kind, {"count": 0, "bytes": 0})
        values["count"] += 1
        values["bytes"] += path.stat().st_size

    hashes: dict[tuple[str, str], list[Path]] = defaultdict(list)
    for path in asset_files:
        if path.suffix.lower() != ".css":
            hashes[(digest(path), path.suffix.lower())].append(path)
    duplicate_groups = [paths for paths in hashes.values() if len(paths) > 1]
    duplicate_savings = sum(
        sum(path.stat().st_size for path in group) - group[0].stat().st_size
        for group in duplicate_groups
    )

    identical_page_groups: list[list[Path]] = []
    for mode in ("desktop", "mobile"):
        mode_hashes: dict[str, list[Path]] = defaultdict(list)
        for page in generated_html:
            if page.relative_to(output).parts[0] != mode:
                continue
            mode_hashes[digest(page)].append(page)
        identical_page_groups.extend(paths for paths in mode_hashes.values() if len(paths) > 1)

    rel = lambda path: path.relative_to(output).as_posix()
    return {
        "schema_version": 1,
        "policy": "report-only; no files were removed",
        "generated_pages": len(generated_html),
        "asset_files": len(asset_files),
        "legacy_incoming_links": sum(incoming.values()),
        "pages_without_legacy_incoming_links": {
            mode: {"count": len(paths), "paths": paths}
            for mode, paths in no_legacy_incoming.items()
        },
        "unreferenced_assets": {
            "count": len(unreferenced_assets),
            "bytes": sum(path.stat().st_size for path in unreferenced_assets),
            "paths": [rel(path) for path in unreferenced_assets],
            "by_extension": dict(sorted(unused_types.items())),
        },
        "duplicate_asset_groups": {
            "count": len(duplicate_groups),
            "potential_savings_bytes": duplicate_savings,
            "groups": [[rel(path) for path in group] for group in duplicate_groups],
        },
        "content_identical_page_groups": {
            "count": len(identical_page_groups),
            "groups": [[rel(path) for path in group] for group in identical_page_groups],
        },
    }


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--report", required=True, type=Path)
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    output = args.output.resolve()
    if not output.is_dir():
        raise SystemExit(f"reference output missing: {output}")
    report = audit(output)
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(
        json.dumps(report, ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8",
        newline="\n",
    )
    unreferenced = report["unreferenced_assets"]
    duplicates = report["duplicate_asset_groups"]
    identical = report["content_identical_page_groups"]
    print("legacy reference migration audit: REPORT ONLY")
    print(f"generated pages: {report['generated_pages']}")
    print(f"unreferenced assets: {unreferenced['count']} ({human_bytes(unreferenced['bytes'])})")
    print(f"duplicate asset groups: {duplicates['count']} ({human_bytes(duplicates['potential_savings_bytes'])})")
    print(f"content-identical page groups: {identical['count']}")
    for mode, values in report["pages_without_legacy_incoming_links"].items():
        print(f"{mode} pages without legacy incoming links: {values['count']}")
    print(f"audit report: {args.report}")


if __name__ == "__main__":
    main()
