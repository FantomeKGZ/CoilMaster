#!/usr/bin/env python3
"""Validate a generated CoilMaster legacy winding-reference import."""
from __future__ import annotations

import argparse
import hashlib
import re
from pathlib import Path
from urllib.parse import unquote, urlsplit

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_OUTPUT = ROOT / "firmware" / "esp32" / "web" / "sites" / "reference"
HTML_SUFFIXES = {".html", ".htm"}
ATTR_RE = re.compile(r"\b(?:href|src)\s*=\s*[\"'](?P<value>.*?)[\"']", re.I)
REFERENCE_PREFIX = "/sites/reference/"


def digest(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def html_count(root: Path) -> int:
    return sum(1 for path in root.rglob("*") if path.is_file() and path.suffix.lower() in HTML_SUFFIXES)


def generated_page_count(output: Path, mode: str) -> int:
    pages = output / mode / "pages"
    return html_count(pages) if pages.exists() else 0


def resolve_reference_url(output: Path, value: str) -> Path | None:
    parts = urlsplit(value)
    if parts.scheme or parts.netloc or not parts.path.startswith(REFERENCE_PREFIX):
        return None
    relative = unquote(parts.path[len(REFERENCE_PREFIX):]).lstrip("/")
    candidate = (output / relative).resolve()
    output_root = output.resolve()
    try:
        candidate.relative_to(output_root)
    except ValueError:
        return Path("/__invalid_reference_escape__")
    if candidate.is_dir():
        index = candidate / "index.html"
        return index if index.exists() else candidate
    return candidate


def validate_pages(output: Path) -> list[str]:
    errors: list[str] = []
    shared_css = output / "shared" / "reference.css"
    shared_js = output / "shared" / "reference.js"
    if not shared_css.is_file():
        errors.append(f"shared stylesheet file missing: {shared_css}")
    if not shared_js.is_file():
        errors.append(f"shared script file missing: {shared_js}")

    for mode in ("desktop", "mobile"):
        mode_pages = output / mode / "pages"
        if not mode_pages.is_dir():
            errors.append(f"generated page directory missing: {mode_pages}")
            continue
        for page in mode_pages.rglob("*"):
            if not page.is_file() or page.suffix.lower() not in HTML_SUFFIXES:
                continue
            try:
                text = page.read_text(encoding="utf-8")
            except UnicodeDecodeError as exc:
                errors.append(f"not utf-8: {page}: {exc}")
                continue
            lowered = text.lower()
            if "class=\"verh\"" in lowered or "class='verh'" in lowered or "images/verh.jpg" in lowered:
                errors.append(f"legacy top logo remains: {page}")
            if "charset=windows-1251" in lowered:
                errors.append(f"legacy charset remains: {page}")
            if "/sites/reference/shared/reference.css" not in text:
                errors.append(f"shared stylesheet missing: {page}")
            if "/sites/reference/shared/reference.js" not in text:
                errors.append(f"shared script missing: {page}")
            for match in ATTR_RE.finditer(text):
                value = match.group("value")
                target = resolve_reference_url(output, value)
                if target is not None and not target.exists():
                    errors.append(f"broken reference link: {page} -> {value}")
    return errors


def duplicate_mode_assets(output: Path) -> list[str]:
    desktop = output / "desktop" / "assets"
    mobile = output / "mobile" / "assets"
    if not desktop.exists() or not mobile.exists():
        return []
    desktop_hashes: dict[str, Path] = {}
    for path in desktop.rglob("*"):
        if path.is_file():
            desktop_hashes[digest(path)] = path
    errors: list[str] = []
    for path in mobile.rglob("*"):
        if not path.is_file():
            continue
        other = desktop_hashes.get(digest(path))
        if other is not None:
            errors.append(f"duplicate asset should be shared: {other} == {path}")
    return errors


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--desktop-source", required=True, type=Path)
    parser.add_argument("--mobile-source", required=True, type=Path)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    output = args.output.resolve()
    desktop_source = args.desktop_source.resolve()
    mobile_source = args.mobile_source.resolve()

    errors: list[str] = []
    for mode, source in (("desktop", desktop_source), ("mobile", mobile_source)):
        source_count = html_count(source)
        output_count = generated_page_count(output, mode)
        if source_count != output_count:
            errors.append(f"{mode} page count mismatch: source={source_count}, generated={output_count}")

    errors.extend(validate_pages(output))
    errors.extend(duplicate_mode_assets(output))

    if errors:
        for error in errors[:100]:
            print(f"ERROR: {error}")
        if len(errors) > 100:
            print(f"ERROR: ... and {len(errors) - 100} more")
        raise SystemExit(1)

    print(f"desktop pages: {generated_page_count(output, 'desktop')}")
    print(f"mobile pages: {generated_page_count(output, 'mobile')}")
    print("legacy winding reference check: OK")


if __name__ == "__main__":
    main()
